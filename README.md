## PSEL PATOS - FIREWALL

#### Jornada

Ao longo da construção dessa firewall, se é que podemos chamar disso, aprendi muito sobre redes e protocolos, algo que não tinha nenhuma experiência.
Em um primeiro momento, tentei compreender o mínimo sobre cada tópico que deveria ser abordado e cheguei a conclusão que o protocolo UDP seria um dos mais fáceis. Por isso, foquei em aprender seu funcionamento, de forma um pouco mais aprofundada.

Após esse período de estudo, comecei a implementar a firewall, primeiro tentando configurar a TUN e me familiarizar com o formato de dados que eu teria que trabalhar, além de estruturar o código com suporte a multi-threads. Mais adiante, tentei implementar o protocolo UDP, sem fragmentação, em primeiro momento, simplesmente validando o checksum. Após terminar essa etapa, passei a elaborar uma estrutura para armazenar os fragmentos, no caso, uma espécie de hash (bem simples) com encadeamento, para tratar colisões de id (do IPV4), e cada node do hash possui uma série de fragmentos já ordenados pelo offset automaticamente. 

Com essa etapa encerrada e funcionando,  parti para implementar regras em tempo real. Para isso usei UDS (Unix Domain Sockets) para possibilitar uma comunicação interna e eficaz além de modelável, já que deixei como socket RAW, permitindo que eu realizasse contato com a firewall durante seu funcionamento. Por mais que a mesma struct foi usada para inserir regras tanto de IP, PORT e palavras, cheguei a conclusão que somente uma árvore trie (ou virtual) que estava utilizando tanto para IP quanto para PORT, embora em diferentes "objetos", não seria capaz de armazenar strings de maneira eficiente devido à sua própria natureza, por isso adicionei uma lista ligada simples que armazena as palavras proibidas e as confere quando necessário, percorrendo-a. 

Um problema que enfrentei foi relativo a remoção, visto que ao fim do programa todas as regras são salvas em disco e se eu removesse uma regra já salva anteriormente por meio da comunicação por UDS ou eu teria que vasculhar o disco e removê-la na hora ou essa ação seria perdida e a regra continuaria em disco, sendo carregada na próxima vez que o programa entrasse em execução. Por esses motivos optei por realizar uma remoção lógica, e, devido ao número baixo de regras, reescrever o arquivo de armazenamento de regras a cada vez que a firewall fosse encerrada, mesmo sabendo dos problemas relativos a esssa ação.

Posteriormente, avancei para o protocolo TCP, o qual encontrei grande dificuldade para compreender seu funcionamento, já que é bem distante do UDP, único que havia implementado até esse momento, mas quis implementar o three-way-handshake de qualquer forma, talvez para entender melhor esse protocolo. Depois que entendi, com o que estava lidando o caminho foi relativamente tranquilo.

Por fim, fui para o ICMP, que em primeiro momento achei que seria moleza, comparado ao TCP, mas me enrolei bem, principalmente para resolver sua fragmentação, que por acaso continuo em dúvida, visto que o wireshark não estava me dizendo o que estava acontecendo, desconfio que seja porque eu não estava escrevendo de volta na TUN, mas sim em outro fd, visto que o IP do meu computador é diferente de 10.0.0.0/8, embora tenha funcionado relativamente bem ao fim.


#### Funcionalidades

Conta com:

- Multi-Threading;
- Suporte a inserção e remoção de regras em tempo real;
- UDP, fragmentado ou não
- TCP, three-way-handshake, respostas com ACK e fechamento de conexões (sem RST)
- ICMP, fragmentado ou não


#### Como usar

Rode no diretório corrente:

```bash
make
sudo ./fire
```

Para controlar regras, em outro terminal, rode (com a firewall rodando):



```bash
sudo ./rule <ADD ou REM> <IP ou PORT ou WORD> <xxx.xxx.xxx/xx ou xxxx ou xxx...> <IN ou OUT ou BOTH>
```

Exemplos:
```bash
sudo ./rule ADD IP 10.0.0.5/24 BOTH (both se refere a tanto a entrada como saída de pacotes)
```

```bash
sudo ./rule ADD PORT 8081 IN
```

```bash
sudo ./rule ADD WORD Hello OUT
```

Existem alguns programas para testes, mas nada de surpreendente.


#### Fontes

https://www.youtube.com/watch?v=5PPfy-nUWIM
https://www.youtube.com/watch?v=VjBDgcNno-Q
https://www.youtube.com/watch?v=tHK20Suz5I4
https://www.youtube.com/watch?v=F27PLin3TV0
https://www.youtube.com/watch?v=EHV0Q0R--Ns
https://networklessons.com/ip-routing/user-datagram-protocol-udp-packet-header
https://ddos-guard.net/blog/udp-protocol-how-it-works
https://www.geeksforgeeks.org/computer-networks/user-datagram-protocol-udp/
https://www.geeksforgeeks.org/computer-networks/calculation-of-tcp-checksum/

Gemini, mas pedi para não fornecer código, principalmente para a indicação de erros e compreensão de como alguns protocolos ou ações deveriam funcionar.
