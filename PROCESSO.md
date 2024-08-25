Já tinha certo conhecimento de socket, mas pulei pra entender de vez cada funcao e valor

O susto foi conhecer que existe um milhão de funções inet_ALGUMACOISA
cada uma retornando algo diferente e com argumentos diferentes
aparentemente elas são um conjunto de funções que transforma endereços
seja vindo de strings: "192.168.0.1" ou de inteiros: 0x100007f -> "127.0.0.1"

```c
// enderecos legiveis aqui querem dizer "xxx.xxx.xxx.xxx"
inet_ntoa() // -> network to ascii -> entao pega enderecos de rede e os torna legiveis
inet_aton() // -> ascii to network -> pega enderecos legiveis 

```

(foquei o estudo apenas em IPv4 entao não sei o quanto disso aqui também é válido para IPv6, por exemplo, ou outros)
existem dois tipos de dados que caracterizam os enderecos aparentemente: `struct in_addr` e `in_addr_t`

entretanto, olhando a definicao vemos que
```c
// file: <netinet/in.h>

/* Internet address.  */
typedef uint32_t in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };
```
OU SEJA! `in_addr_t` é apenas um número inteiro e `struct in_addr` contém apenas esse numero inteiro, ou seja, pelo que parece o struct in_addr apenas dificulta nossa vida!! Criando mais uma variável pra designar a mesma informação. Minha suposição é de que em algum caso este struct deve ser realmente útil.

Para fim da compreensão das funcoes `inet_algumacoisa` temos que esse struct faz com que existam mais funções já que temos que converter de `const char*` para `in_addr_t` e para `struct in_addr` e vice-versa

---
Outra coisa que descobri é que os endereços são separados em parte de rede `network` e parte da rede local `local`, ou seja, um ip "127.0.0.1" é separado em:

```c
"127.0.0" // network
"1"       // local


// isso também acaba gerando mais funcoes inet_ALGUMACOISA
```
---
Já a função accept aceita dois argumentos relacionados com mais um struct que eu não conheço ent bora de documentação e ler .h

Aparebtemebte `struct sockaddr_in` é específico para o protocolo IPv4, enquando que `struct sockaddr` é um struct mais geral e isso seria uma espécie de "Polimorfimsmo" que eu não lembro mais o que significa