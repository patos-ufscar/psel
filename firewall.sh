#!/bin/bash
# FIREWALL SYSTEM INITIALIZATION SCRIPT (.sh)
# =====================================================================
# DICA: Este script deve ser executado como ROOT (sudo ./firewall.sh start)
# Certifique-se de dar permissão de execução: chmod +x firewall.sh

#EXT_INT="eth0"
#FAKE_IP="10.0.0.2"
TUN_INT="tun0"
TUN_IP="10.0.0.1"
TUN_NET="10.0.0.0/24"
BIN_PATH="./fwall"
PID_FILE="/tmp/firewall.pid"

check_root() {
  if [ "$EUID" -ne 0 ]; then #verifica se o processo tem as permissoes necessarias para a execucao
    echo "[X] Este script precisa ser executado com root."
    echo "        Use: sudo $0 $1"
    exit 1
  fi
}

check_binary() {
  if [ ! -f "$BIN_PATH" ]; then # verifica se o binario foi criado e esta no local correto
    echo "[X] Binário do firewall não encontrado em '$BIN_PATH'."
    echo "     Compile o projeto antes de iniciar (ex: make)."
    exit 1
  fi

  if [ ! -x "$BIN_PATH" ]; then # verifica se o binario tem as permissoes necessarias
    echo "[X] '$BIN_PATH' Existe mas não tem permissão de execução."
    echo "    Rode: chmod +x $BIN_PATH"
    exit 1
  fi
}
#verifica se tem as dependencias necessarias
check_dependencies() {
  for cmd in ip; do # atualmente o loop so roda uma vez, mas caso no futuro mais comandos sejam usados basta adicionalos ex: "for cmd in ip, iptables, traceroute, etc...""
    if ! command -v "$cmd" &>/dev/null; then
      echo "[X] Comando '$cmd' não encontrado. Instale o pacote correspondente (ex: iproute2)."
      exit 1
    fi
  done
}
# verifica se o processo ta rodando
is_running() {
  if [ -f "$PID_FILE" ]; then
    local pid
    pid=$(cat "$PID_FILE" 2>/dev/null)
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
      return 0
    fi
  fi
  return 1
}

start_firewall() {
  check_root "start"
  check_binary
  check_dependencies

  if is_running; then
    echo "[!] O firewall já está rodando (PID: $(cat "$PID_FILE"))."
    echo "    use '$0 stop' antes de iniciar de novo, ou '$0 restart'."
    exit 1
  fi

  echo "[+] Inicializando infraestrutura do Firewall..."

  if ! echo 0 >/proc/sys/net/ipv4/ip_forward; then
    echo "[X] falha ao desativar ip_forward. abortando"
    exit 1
  fi

  if ! ip link show "$TUN_INT" &>/dev/null; then # verifica se a interface TUN ja existe
    echo "[+] Criando interface virtual $TUN_INT..."
    if ! ip tuntap add mode tun dev "$TUN_INT"; then
      echo "Falha ao criar a interface $TUN_INT"
      exit 1
    fi
  else
    echo "interface $TUN_INT ja existe, reutilizando."
  fi

  echo "[+] Configurando IP $TUN_IP na interface $TUN_INT..."
  ip addr add "$TUN_IP/24" dev "$TUN_INT" 2>/dev/null # ignora erro se o ip ja tiver atribuido

  if ! ip link set dev "$TUN_INT" up; then # ativa a interface
    echo "[X] falha ao ativar a interface $TUN_INT"
  fi

  echo "[+] Definindo rotas de armadilha para o Tarpit..."

  ip route add "$TUN_NET" dev "$TUN_INT" proto static scope link 2>/dev/null #  cria uma rota para a interface TUN, ignora o erro se a rota ja existir

  # Redireciona o tráfego que entra na eth0 na porta 80 direto para o IP do seu firewall na tun0
  #iptables -t nat -A PREROUTING -i $EXT_INT -p tcp --dport 80 -j DNAT --to-destination $FAKE_IP

  if ! echo 1 >/proc/sys/net/ipv4/conf/$TUN_INT/rp_filter; then
    echo "[!] Aviso: falha ao ajustar rp_filter em $TUN_INT (nao fatal)"
  fi

  echo "[✓] Ambiente pronto."
  echo "[+] Iniciando o binário do firewall ($BIN_PATH)..."
  "$BIN_PATH" &
  local bin_pid=$!

  # da um tempo pra confirmar que o processo nao morreu ao iniciar
  # ex: falha ao abrir /dev/net/tun, arquivos de regras ausente e etc
  sleep 0.5
  if ! kill -0 $bin_pid 2>/dev/null; then
    echo "[X] o binario do firewall encerrou logo apos iniciar. Verifique os logs/saida acima"
    rm -f $PID_FILE
    exit 1
  fi

  echo "$bin_pid" >"$PID_FILE"
  echo "[✓] Firewall rodando em background (PID: $bin_pid."
}

stop_firewall() {
  check_root "stop"
  echo "[-] Desativando infraestrutura do Firewall..."

  if is_running; then
    local pid
    pid=$(cat $PID_FILE)
    echo "[-] Encerrando o processo do firewall..."
    kill "$pid" 2>/dev/null

    for _ in $(seq 1 10); do
      kill -0 "$pid" 2>/dev/null || break
      sleep 0.5
    done

    if kill -0 "$pid" 2>/dev/null; then
      echo "[!] processo nao respondeu a tempo, forcando encerramento(SIGKILL)"
      kill -9 "$pid" 2>/dev/null
    fi

    rm -f $PID_FILE
  else
    echo "[!] Processo não encontrado ou já encerrado."
    rm -f $PID_FILE
  fi

  #echo "[-] Removendo regras de redirecionamento do iptables..."
  #iptables -t nat -D PREROUTING -i $EXT_INT -p tcp --dport 80 -j DNAT --to-destination $FAKE_IP 2>/dev/null

  #remove a interface tun
  if ip link show "$TUN_INT" &>/dev/null; then
    echo "[-] Removendo interface virtual $TUN_INT..."
    ip link set dev "$TUN_INT" down 2>/dev/null
    if ! ip tuntap del mode tun dev "$TUN_INT" 2>/dev/null; then
      echo "[!]Aviso: falha ao remover a interface $TUN_INT (pode ja ter sido removida)"
    fi
  fi

  echo "[✓] Sistema restaurado ao estado original."
}

case "$1" in
start)
  start_firewall
  ;;
stop)
  stop_firewall
  ;;
restart)
  stop_firewall
  sleep 1
  start_firewall
  ;;
status)
  if is_running; then
    echo "[] Firewall rodando (PID: $(cat "$PID_FILE"))"
  else
    echo "[!] Firewall nao esta rodando."
  fi
  ;;
*)
  echo "Uso correto: $0 {start|stop|restart}"
  exit 1
  ;;
esac
