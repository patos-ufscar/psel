# Reverse Proxy p/ Servidor de arquivos


Esse repositório possui dois programas implementados a fim de participar do processo seletivo do grupo patos/pombos da UFSCar, eles consistem em:
- Um Reverse Proxy implementado em Java, capaz de parsear requisições e fornecer arquivos locais;
- Um backend implementado em C, capaz de receber requisições HTTP e entregar arquivos;


## Compilação e instalação


Para a instalação completa do software desse repositório, é necessário fazer a compilação de ambos os programas separadamente.


### backend_c


Para inicializar o backend é necessário compilar e executar o main.c


- O programa é compatível tanto com Windows quanto Linux


Execute no terminal:
```
gcc main.c -o main && ./main
```


Ou no cmd:
```
gcc main.c -o main && .\main
```


Com isso o programa estará em execução em `localhost:8081`, caso queira mudar a porta, ela pode ser encontrada como um `#define` em `main.c`


### reverse-proxy-file-server


Para inicializar o Reverse Proxy é necessário compilar o arquivo Java.
 - Nas versões mais recentes do java, podemos compilar e executar um arquivo .java apenas utilizando o comando `java` sem a necessidade de executar `javac`


 Portanto, deve-se executar no cmd, ou terminal:
 ```
 java -cp .\reverse-proxy-file-server\resources .\reverse-proxy-file-server\src\main\java\com\ufscar\psor\Main.java
 ```
Com isso o programa estará em execução em `localhost:8080`, caso queira mudar a porta, ela pode ser encontrada como um `private static final` em `Main.java`

 ## Teste e utilização


 Após inicializar ambos os programas, pode-se ler no terminal em qual porta eles estão sendo executados
 - `8080` e `8081` por padrão
 
 Então, pode-se utilizar qualquer navegador e acessar `http://localhost:8080/` para ter acesso a Index page que te de acesso a navegar pelos arquivos.
 - É possível também fazer requests manualmente por ferramentas como netcat


 Os terminais nos quais os programas estão sendo executados fornecem informações sobre os clientes que estão conectando-se.


 ## Fontes


 Grande parte do conteúdo puro sobre HTTP e requests foram adquiridos nas [aulas do patos](https://www.youtube.com/watch?v=iuwSYRdxKjQ), para além disso
- [Wikipedia](https://en.wikipedia.org/wiki/HTTP), para trabalhar com requests
- [StackOverflow](https://stackoverflow.com/), para grande partes dos problemas com Java
- [Oracle](https://docs.oracle.com/en/), para encontrar as bibliotecas necessárias


 ## Experiência pessoal sobre o projeto


Antes de iniciar o desenvolvimento desse projeto eu não tinha conhecimento algum sobre redes, muito menos sobre API's e Proxys, eu estava apenas na fase "da faculdade" de fazer programas que fazem contas ou estruturas de dados. Confesso que o início do desenvolvimento foi desafiador, um pouco até mais do que eu esperava, e um pouco assustador, devido a mudança de perspectiva do que estava sendo feito, o foco não era mais "executar" e sim integrar diferentes funcionalidades.


Porém, apesar de ter sido ameaçador no início, foi um ótimo projeto para eu finalmente tirar a pulga atrás da orelha que eu tinha sobre quando as matérias da faculdade "fariam sentido" juntas. Eu aproveitei essa oportunidade de fazer um programa que ja era integrado em múltiplas camadas para tentar fazer pela primeira fez um projeto que envolvia mais de uma linguagem de programação, o que foi realmente o ponto alto do projeto na minha opnião, ver dois programas que eram completamente independentes até o momento, funcionando em conjunto em tempo real, em uma aplicação web.


No geral acho que foi o empurrão que faltava para eu entrar nas tecnologias mais novas e reais de programação e sair um pouco da caixa da faculdade, por mais que tenha dado bastante trabalho valeu bastante a pena, para ter a liberdade de testar coisas novas sem a pressão da avaliação da universidade.


### Planos Futuros


Algumas coisas que eu gostaria de ter implementado porém não houve tempo foram:
- O parseamento "correto" das requisições de proxy, ao invés de utilizar de hard-coding para parsear a request ao back end
- Permitir o acesso ao um servidor de arquivos propriamente dito quando acessar o proxy, ao invés de apenas um arquivo
- Fazer mais controles de erro e conexões relacionados ao servidor Proxy, como limite de conexões, conexões válidas etc...


Algo também importante porém não muito relacionado ao projeto original é:
- Abusar mais da orientação a objeto, como não programava em nada além de C a um tempo, acabei não utilizando muitas funcionalidades que o Java proporciona (apenas 1 classe...).

