# PATOS/POMBO PSEL 2.0
# Userspace C Firewall

Um firewall customizado operando em espaço de usuário (*userspace*), escrito inteiramente em C. O projeto intercepta, analisa e filtra tráfego de rede capturado através de uma interface virtual `tun0`, utilizando regras de roteamento avançadas no Linux.

## Visão Geral 
O sistema funciona desviando o tráfego de rede (via DNAT com `iptables`) para uma interface virtual (TUN). O programa em C lê os pacotes IP crus desta interface, toma decisões de bloqueio ou liberação com base em regras predefinidas, recalcula as integridades matemáticas dos cabeçalhos e devolve os pacotes processados para o sistema operacional.

O projeto está modularizado em três componentes principais:

### 1. Main (`src/main.c`)
É o ponto de entrada do programa. Responsável por fazer a ponte entre o sistema operacional e a lógica de filtragem.
* **Função:** Abre o *file descriptor* da interface `tun0`.
* **Fluxo:** Implementa o loop principal infinito que invoca o `read()` para escutar os pacotes chegando da rede.
* **Ação:** Envia o pacote capturado para o módulo de regras e, dependendo do veredito (ACCEPT), usa o `write()` para injetar o pacote de volta na interface, permitindo que ele siga seu destino.

### 2. Rules (`src/rules.c`) 
Onde a lógica de negócios do firewall reside. 
* **Função:** Faz o *parsing* (desmontagem) dos bytes crus nos cabeçalhos corretos (IPv4, TCP, UDP, ICMP).
* **Fluxo:** Analisa IPs de origem/destino e portas. 
* **Ação:** Aplica a política de segurança, retornando `ACCEPT` (deixa passar), `DROP` (ignora silenciosamente o pacote) ou `TARPIT` (Ignora o pacote e deixa a origem em espera por um periodo de tempo).

### 3. Checksum (`src/checksum.c`) 
O componente de validação e integridade.
* **Função:** Calcula o *checksum* (soma de verificação) dos pacotes.
* **Motivação:** Quando um pacote passa pela interface TUN ou sofre qualquer modificação no userspace, o kernel do Linux invalida seu checksum original. 
* **Ação:** Aplica o algoritmo de complemento de um para recalcular os checksums de IP e TCP/UDP. Se isso não for feito perfeitamente, os pacotes são descartados como "corrompidos" pelos roteadores ou pelo próprio kernel na saída.

### Instalação
Certifique-se que vc tem as dependências necessárias, caso contrário execute o comando abaixo:
```bash
sudo apt install build-essential gcc make iptables iproute2 -y
```
Com as dependências instaladas clone o repositório e inicialize o firewal
```bash
git clone https://github.com/itzxw/psel
cd psel
git checkout my-firewall
```
```bash
chmod +x firewall.sh
make
sudo ./firewall.sh start
```
Não esqueça de colocar a interface que vc deseja redirecionar o tráfego no firewall.sh (por padrão está como eth0) e de adicionar suas regras no arquivo conf.rc
caso queira checar se o programa está funcionando corretamente uso o comando abaixo:
```bash
sudo tcpdump -i tun0 -n -vv
```

##  Estrutura do Projeto

```text
.
├── includes/
│   └── checksum.h
|   └── rules.h     # Declarações de funções, macros e estruturas comuns
├── src/
│   ├── main.c         # Ponto de entrada e manipulação da interface TUN
│   ├── rules.c        # Lógica de inspeção profunda de pacotes (DPI)
│   └── checksum.c     # Rotinas matemáticas de recálculo de cabeçalhos
├── conf.rc            # Regras do firewall
├── firewall.sh        # Script de inicialização (Roteamento, NAT e IP Forwarding)
├── Makefile           # Automação da compilação
└── README.md


