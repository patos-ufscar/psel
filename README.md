# PATOS/POMBO PSEL 2.0
![GIF](https://media1.tenor.com/m/wJoIg9kMUB0AAAAC/duck-spin.gif)
# Userspace C Firewall

Um firewall customizado operando em espaço de usuário (*userspace*), escrito inteiramente em C. O projeto intercepta, analisa e filtra tráfego de rede capturado através de uma interface virtual `tun0`, utilizando regras de roteamento avançadas no Linux.

## Visão Geral 
Diferente de um firewall tradicional baseado em `iptables`/`nftables` (que roda dentro do kernel), este projeto implementa toda a lógica de filtragem em um processo de userspace (`fwall`), escrito em C. O sistema operacional entrega os pacotes para esse processo através de uma interface de rede virtual (TUN), o processo decide o que fazer com cada pacote (bloquear, deixar passar, ou "prender" a conexão via tarpit), e devolve o resultado — tudo isso sem exigir nenhuma modificação no kernel.

### Como funciona uma interface TUN

Uma interface TUN ("network TUNnel") é uma interface de rede virtual criada em software, sem hardware físico por trás. Ela opera na camada 3 (rede) do modelo OSI — ou seja, o programa que a controla lê e escreve pacotes IP "crus" (sem cabeçalho Ethernet), diferente de uma interface TAP, que opera na camada 2.

**O fluxo básico é:**

+ O programa abre `/dev/net/tun` e usa `ioctl(TUNSETIFF)` para registrar uma nova interface (ex: tun0) no kernel, associada ao seu file descriptor.
+ O kernel passa a tratar `tun0` como qualquer outra interface de rede: ela pode receber um IP, ter rotas associadas, aparecer no ip link, etc.
+ Sempre que o kernel decide (com base na tabela de rotas) que um pacote deve ser enviado por `tun0`, em vez de transmiti-lo por um cabo/rádio, ele entrega esse pacote para o processo que abriu o file descriptor, como se fosse um `read()` de um arquivo comum.
+ O processo pode inspecionar, modificar, descartar ou devolver esse pacote com um `write()` no mesmo file descriptor — e o kernel trata esse `write()` como se o pacote tivesse "chegado" pela interface.

## Escopo e limitações importantes

Este firewall filtra apenas o tráfego que o kernel decide rotear pela interface `tun0` — não todo o tráfego da máquina.
Isso é uma decisão de escopo relevante para entender o projeto corretamente:

+ O script `firewall.sh` cria a interface `tun0` com o IP `10.0.0.1/24` e adiciona uma rota estática apenas para a rede `10.0.0.0/24`.
+ Isso significa que **somente pacotes destinados a essa sub-rede específica** são roteados para dentro do programa e, portanto, avaliados pelas regras do `conf.rc`.
+ Tráfego para qualquer outro destino (por exemplo, uma requisição para um servidor na internet) continua seguindo a rota padrão do sistema, saindo pela interface física (`eth0`, Wi-Fi, etc.) — **sem nunca passar pelo firewall**.

Em outras palavras: o "default deny" implementado no código (pacotes sem regra correspondente são descartados) é válido **dentro do universo de pacotes que o programa efetivamente recebe**, e não representa uma política de segurança para toda a máquina. Para que o firewall interceptasse todo o tráfego de saída de um host, seria necessário substituir a rota padrão (`0.0.0.0/0`) do sistema para apontar para `tun0` e implementar encaminhamento (forwarding) real dos pacotes liberados de volta para a rede — o que está fora do escopo atual do projeto.

## Arquitetura do projeto
 
O código é dividido em três módulos:
 
| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Loop principal: criação da interface TUN, leitura de pacotes, aplicação das regras, encaminhamento/descarte, logging. |
| `rules.c` / `rules.h` | Parsing do arquivo de configuração (`conf.rc`) e construção da lista de regras em memória. |
| `checksum.c` / `checksum.h` | Cálculo manual dos checksums de IP, TCP e ICMP, necessários sempre que um pacote é modificado antes de ser reenviado. |
| `firewall.sh` | Orquestração da infraestrutura de rede (criação/remoção da interface TUN, IP, rotas) e gerenciamento do ciclo de vida do processo (`start`/`stop`/`restart`/`status`), incluindo validações de permissão e tratamento de erros. |

### Instalação
Certifique-se que vc tem as dependências necessárias, caso contrário execute o comando abaixo:
```bash
# Debian/Ubuntu
sudo apt install build-essential iproute2
 
# Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install iproute
 
# Arch
sudo pacman -S base-devel iproute2
 
# dependências opcionais para testes (Debian/Ubuntu)
sudo apt install netcat-openbsd tcpdump python3-scapy
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
## Testando o firewall
 
Como o firewall só enxerga tráfego roteado para a rede `10.0.0.0/24` (ver [Escopo e limitações](#escopo-e-limitações-importantes)), o tráfego de teste precisa ser gerado explicitamente contra a interface `tun0` ou o IP `10.0.0.1`. Algumas formas de fazer isso:
 
**Ping (testa a ação `ALLOW` com resposta ICMP):**
```bash
ping -c 3 10.0.0.1
```
 
**Conexão TCP (testa `DENY`/`TARPIT`, dependendo da regra para o IP de origem):**
```bash
nc 10.0.0.1 80
```
 
**Observando o tráfego bruto na interface:**
```bash
sudo tcpdump -i tun0 -vv
```
 
**Gerando pacotes customizados** (IP de origem forjado, payload específico, flags TCP arbitrárias), útil para testar regras e o filtro de payload malicioso de forma controlada, usando [Scapy](https://scapy.net/):
```python
from scapy.all import *
send(IP(src="10.0.0.99", dst="10.0.0.1")/TCP(dport=80, flags="S"), iface="tun0")
```
## Experiência com o PSEL
Minha experiência no geral foi bem interessante, acabei demorando pra finalizar o projeto por conta da disponibilidade, eu fazia uma parte e depois só ia completar depois de alguns dias, mas no fim deu certo.Acho que o mais daora foi poder ver a força que o C tem nesse cenário de manipulação de pacotes e de redes no geral, até pensei em adicionar mais algumas funcionalidades, como o tratamento do IPV6 e algumas técnicas anti-ddos, mas decidir por talvez implementar isso futuramente, até porque já tava demorando demais pra finalizar o código.No geral deu pra aprender bastante e foi uma experiência bem daora.

##  Referências
- [RFC 1071 - Computing the Internet checksum](https://datatracker.ietf.org/doc/html/rfc1071)
- https://www.geeksforgeeks.org/computer-networks/tcp-ip-packet-format/
- https://stackoverflow.com/questions/75261549/setting-an-ip-address-to-a-tun-in-c
- https://www.linkedin.com/pulse/how-i-built-private-ip-network-using-tun-interface-linux-alan-guldc/
- [Tarpitting (LaBrea Tarpit)](https://en.wikipedia.org/wiki/Tarpit_(networking))





