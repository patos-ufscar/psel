# PATOS/POMBO PSEL 2.0
# Userspace C Firewall

Um firewall customizado operando em espaço de usuário (*userspace*), escrito inteiramente em C. O projeto intercepta, analisa e filtra tráfego de rede capturado através de uma interface virtual `tun0`, utilizando regras de roteamento avançadas no Linux.

## Visão Geral 
O sistema funciona desviando o tráfego de rede (via DNAT com `iptables`) para uma interface virtual (TUN). O programa em C lê os pacotes IP crus desta interface, toma decisões de bloqueio ou liberação com base em regras predefinidas.

O projeto está modularizado em três componentes principais:

### 1. Main (`src/main.c`)
Análise dos pacotes
* **Função:** Abre o *file descriptor* da interface `tun0`.
* **Fluxo:** Implementa o loop principal infinito que invoca o `read()` para escutar os pacotes chegando da rede.
* **Ação:** Envia o pacote capturado para o módulo de regras e, dependendo do veredito (ACCEPT), usa o `write()` para injetar o pacote de volta na interface, permitindo que ele siga seu destino.

### 2. Rules (`src/rules.c`) 
Leitura das Rules 
* **Função:** Faz o *parsing* (desmontagem) dos bytes crus nos cabeçalhos corretos (IPv4, TCP, UDP, ICMP).
* **Fluxo:** Analisa IPs de origem/destino e portas. 
* **Ação:** Aplica a política de segurança, retornando `ACCEPT` (deixa passar), `DROP` (ignora silenciosamente o pacote) ou `TARPIT` (Ignora o pacote e deixa a origem em espera por um periodo de tempo).

### 3. Checksum (`src/checksum.c`) 
Calcula o checksum para a funcionalidade `TARPIT`
* **Função:** Calcula o *checksum* (soma de verificação) dos pacotes.
* **Motivo:** Quando um pacote caí na regra `TARPIT` precisamos enviar para a origem um pacote tcp SYN-ACK com o parametro windows=0, porém, como fazemos uma modificação nos dados daquele pacote o checksum original já não é mais válido, sendo assim necessário calcular um novo.  
* **Ação:** Aplica o algoritmo de complemento de um para recalcular os checksums de IP e TCP/UDP.

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

##  Referências
- [RFC 1071 - Computing the Internet checksum](https://datatracker.ietf.org/doc/html/rfc1071)
- https://stackoverflow.com/questions/75261549/setting-an-ip-address-to-a-tun-in-c
- https://www.linkedin.com/pulse/how-i-built-private-ip-network-using-tun-interface-linux-alan-guldc/





