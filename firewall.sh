#!/bin/bash
# FIREWALL SYSTEM INITIALIZATION SCRIPT (.sh)
# =====================================================================
# DICA: Este script deve ser executado como ROOT (sudo ./firewall.sh start)
# Certifique-se de dar permissão de execução: chmod +x firewall.sh

TUN_INT="tun0"
FAKE_IP="10.0.0.2"
TUN_IP="10.0.0.1"
TUN_NET="10.0.0.0/24"
EXT_INT="enp0s3"
BIN_PATH="./fwall"
PID_FILE="/tmp/firewall.pid" 

start_firewall() {
    echo "[+] Inicializando infraestrutura do Firewall..."

    echo 1 > /proc/sys/net/ipv4/ip_forward

    if ! ip link show $TUN_INT &>/dev/null; then
        echo "[+] Criando interface virtual $TUN_INT..."
        ip tuntap add mode tun dev $TUN_INT
    fi

    echo "[+] Configurando IP $TUN_IP na interface $TUN_INT..."

    ip addr add $TUN_IP/24 dev $TUN_INT 2>/dev/null
    ip link set dev $TUN_INT up


    echo "[+] Definindo rotas de armadilha para o Tarpit..."

    ip route add $TUN_NET dev $TUN_INT proto static scope link 2>/dev/null

    # Redireciona o tráfego que entra na eth0 na porta 80 direto para o IP do seu firewall na tun0
    iptables -t nat -A PREROUTING -i $EXT_INT -p tcp --dport 80 -j DNAT --to-destination $FAKE_IP

    echo 1 > /proc/sys/net/ipv4/conf/$TUN_INT/rp_filter

    echo "[✓] Ambiente pronto."

    echo "[+] Iniciando o binário do firewall ($BIN_PATH)..."
    $BIN_PATH &
    
    # Salva o número do processo (PID) no nosso arquivo temporário
    echo $! > $PID_FILE 
    echo "[✓] Firewall rodando em background (PID: $(cat $PID_FILE))."
}

stop_firewall() {
    echo "[-] Desativando infraestrutura do Firewall..."

    if [ -f $PID_FILE ]; then
        echo "[-] Encerrando o processo do firewall..."
        kill $(cat $PID_FILE) 2>/dev/null
        rm -f $PID_FILE
    else
        echo "[!] Processo não encontrado ou já encerrado."
    fi

    echo "[-] Removendo regras de redirecionamento do iptables..."
    iptables -t nat -D PREROUTING -i $EXT_INT -p tcp --dport 80 -j DNAT --to-destination $FAKE_IP 2>/dev/null

    if ip link show $TUN_INT &>/dev/null; then
        echo "[-] Removendo interface virtual $TUN_INT..."
        ip link set dev $TUN_INT down
        ip tuntap del mode tun dev $TUN_INT
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
    *)
        echo "Uso correto: $0 {start|stop|restart}"
        exit 1
        ;;
esac
