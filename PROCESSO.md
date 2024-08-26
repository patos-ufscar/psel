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
---
Agora é necessário processar os requests e enviar o arquivo desejado, como no caso o servidor é apenas de arquivos o trabalho é mais fácil do que um ter que processar parte da página (a informação é estática, não precisamos mudar nada, apenas ler e enviar)

Decidi separar essa parte em primeiro processar alguns requests inválidos e depois criar a parte que envolvesse os arquivos, para isso foi necessário fazer o _parsing_ dos requests:

Foi sugerido, pelo grupo PATOS um código de github com uma função chamada `strsplit` que auxiliaria e muito nessa parte, entretanto, fuçando na biblioteca `<string.h>` tomei conhecimento da função `strtok` que faz função semelhante à `strsplit`.

Existe também a função `strtok_r`, então fui caçar qual a diferença entre as duas:
da man page temos:
        
```
  ┌───────────────────────┬───────────────┬───────────────────────┐
  │ Interface             │ Attribute     │ Value                 │
  ├───────────────────────┼───────────────┼───────────────────────┤
  │ strtok()              │ Thread safety │ MT-Unsafe race:strtok │
  ├───────────────────────┼───────────────┼───────────────────────┤
  │ strtok_r()            │ Thread safety │ MT-Safe               │
  └───────────────────────┴───────────────┴───────────────────────┘

  ...


  •  The strtok() function uses a static buffer while parsing, so it's not thread safe.  Use strtok_r() if this matters to you.
```

Então parece que a função strtok_r é de certa forma mais segura que strtok por ter esse atributo de "MT-Safe"?, o que quer dizer ser MT-Safe:

De acordo com a man page de attributes(7):

### MT-Safe
> MT-Safe or Thread-Safe functions are **SAFE TO CALL IN THE
  PRESENCE OF OTHER THREADS.**
  MT, in MT-Safe, stands for
  Multi Thread.

  [...]

### MT-Unsafe
> MT-Unsafe functions are **NOT SAFE TO CALL IN A
      MULTITHREADED PROGRAMS.**

Ou seja, `strtok` não deve ser utilizada se o programa utilizar várias threads, atualmente isso não é um problema, entretanto planejo utilizar multithreading neste servidor num futuro próximo então vou construir as funções de parsing utilizando `strtok_r`