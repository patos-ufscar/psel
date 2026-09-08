# PATOS/POMBO PSEL 2.0
# Userspace C Firewall

Um firewall customizado operando em espaço de usuário (*userspace*), escrito inteiramente em C. O projeto intercepta, analisa e filtra tráfego de rede capturado através de uma interface virtual `tun0`, utilizando regras de roteamento avançadas no Linux.

## Visão Geral 
O sistema funciona desviando o tráfego de rede da interface fornecida(usando o comando `route`) para uma interface virtual (TUN). O programa em C lê os pacotes IP crus desta interface, toma decisões de bloqueio ou liberação com base em regras predefinidas.

O projeto está modularizado em três componentes principais:

### 1. Main (`src/main.c`)
* **Função:** Ler os dados da interface tun e tomar a decisão adequada com base naquela `Rule` fornecida e na estrutura/conteúdo daquele pacote recebido.

### 2. Rules (`src/rules.c`) 
* **Função:** Vai ler o arquivo de configuração e armazenar aquela `Rule` específica, determinando todos os elementos daquela instrução(Ip, Ação, Máscara).

### 3. Checksum (`src/checksum.c`) 
* **Função:** Calcula o *checksum* (soma de verificação) dos pacotes, que vai ser necessária caso tenhamos que modificar algum pacote e mandá-lo de volta pra rede.

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
caso queira checar se o programa está funcionando corretamente use o comando abaixo:
```bash
sudo tcpdump -i tun0 -n -vv
```
## Experiência com o PSEL
Minha experiência no geral foi bem interessante, acabei demorando pra finalizar o projeto por conta da disponibilidade, eu fazia uma parte e depois só ia completar depois de alguns dias, mas no fim deu certo.Acho que o mais daora foi poder ver a força que o C tem nesse cenário de manipulação de pacotes e de redes no geral, até pensei em adicionar mais algumas funcionalidades, como o tratamento do IPV6 e algumas técnicas anti-ddos, mas decidir por talvez implementar isso futuramente, até porque já tava demorando demais pra finalizar o código.No geral deu pra aprender bastante e foi uma experiência bem daora(dito isso, não faça um firewall em C porque vai fazer vc querer programar em C pra sempre).
##  Referências
- [RFC 1071 - Computing the Internet checksum](https://datatracker.ietf.org/doc/html/rfc1071)
- https://stackoverflow.com/questions/75261549/setting-an-ip-address-to-a-tun-in-c
- https://www.linkedin.com/pulse/how-i-built-private-ip-network-using-tun-interface-linux-alan-guldc/





