## Introdução
Este é o processo seletivo para ingresso no grupo PATOS. Optei por fazer o load balancer em Go para conhecer melhor a linguagem e por conta da concorrência (goroutines). Apesar de ter ficado simples, foi um projeto bem legal de se fazer e aprendi um pouco sobre Go.
## Instalação
Clone o repositório
```sh
git clone -b psel --single-branch https://github.com/gdefaria/patos-psel-gdfaria
```

Opcional: compilar para executável
```sh
patos-psel-gdfaria$ go build
```

Configure o load balancer em `config.json`
```json
{
	"port": 8000,
	"strategy": "LeastConnections", // veja as estratégias em balancer/strategies.go
	"servers": [
		"host:port",
		"host:port",
		...
	]
}
```

Execute
```sh
patos-psel-gdfaria$ go run .

# Ou, caso tenha compilado
patos-psel-gdfaria$ ./loadbal
```
## Principais recursos utilizados
- https://go.dev/doc/tutorial/getting-started: aprender Go.
- https://zetcode.com/golang/socket/: ver como são sockets em Go.
- Adam Woodbeck - Network Programming with Go: acabei não usando tanto, mas deu para pegar uma visão geral.
- Muito perplexity.ai para tirar dúvidas.

## Progresso
### Antes de 02/04
Comecei a documentar o processo hoje, mas vou resumir o que fiz até aqui.

Decidi usar Go para o load-balancer, principalmente, pelos seguintes motivos:
- Quero aprender Go.
- A linguagem lida muito bem com concorrência - algo essencial para uma aplicação que lida com multiplos clientes ao mesmo tempo.
- Possui uma sintaxe pouco verbosa, normalmente flat e fácil de tratar erros. Isso reduz a chance do código virar bagunça.

**Importante:** como dito, estou usando o projeto para aprender Go, então não espere código de qualidade. Na realidade, espere o oposto. Com o tempo vou melhorando e atualizando o projeto.

Comecei passando pela documentação da linguagem e testando um pouco. Dei uma lida sobre as técnicas mais comuns de balanceamento em load-balancers e pretendo implementar uma classe que contenha ao menos o balanceamento estático de ciclar os servidores e outro com base nas conexões abertas.

A ideia final é que seja possível compilar um binário que permita configurar uma situação real, ainda que de forma completamente amadora. Isso deve incluir um arquivo de configuração no mesmo diretório que contenha a listagem dos servidores e a técnica de balanceamento. Algo assim:
```
/project
	balancer  # binário compilado
	balancer.conf.[toml,json,txt]  # arquivo de configuração, ainda não sei qual formato vou usar, mas isso não é relevante
	
$ ./balancer
> Balancer is listening at <host>:<port> 
```

Ainda não sei como vou lidar com logs, mas isso não será feito na primeira versão.

Depois, comecei a experimentar com sockets, fiz um MITM simples e logo percebi o que escrevi em **02/04**.

### 02/04
Percebi que o maior desafio seria, na verdade, a criação da reverse proxy que intermedia a conexão entre o cliente e o servidor. Uma vez que a reverse proxy funcione de maneira sólida, o balanceamento (escolha do servidor adequado) não deve ser trabalhoso.

Inicialmente, para a criação da reverse proxy, pensei em receber a requisição HTTP completa e repassar para o servidor, e vice-versa com a resposta. No entanto, isso envolve algumas complicações desnecessáras:
- Parser HTTP para descobrir quando a requisição chegou por completa. Como quero suportar todos os métodos HTTP, seria preciso parsear tanto o método quanto o content-length que pode ou não estar incluso.
- Esperar a requisição por completa e repassar ao próximo peer pode resultar em latência desnecessária.
- A partir do HTTP/1.1, há a implementação nativa da flag keep-alive (que pode ou não estar presente) para não fechar a conexão TCP após a resposta. Não quero escolher lidar com isso quando posso escolher não lidar.

Assim, a ideia é criar duas streams TCP concorrentes, uma para o servidor e outra para o cliente. A proxy simplesmente replica quaisquer atualizações na saída de uma stream para a entrada de outra, e vice-versa. Dessa forma:
- A implementação é muito mais simples.
- Não preciso lidar com as nuances do TCP, essa responsabilidade é transferida unicamente para os peers.
- Não preciso decidir quando fechar a conexão, posso apenas esperar pela tentativa dos peers (e talvez adicionar algum timeout por segura).
- A latência possivelmente será menor, visto que as duas sockets são sincronizadas quase que em tempo real.
-  Considerando o fato de ser uma sincronização TCP, funcionaria para todos os protocolos que utilizam TCP, não apenas para HTTP. O servidor tem total liberdade e flexibilidade para decidir como lidar com essas requisições, da mesma forma que ocorre se não houver um intermediador.

Desvantagems:
- Filtros na comunicação ficam muito mais complicados, já que a proxy não terá a requisição completa antes de já ter roteado boa parte do pacote.
- Lidar com erros de envio de segmentos fica mais complicado e requer uma implementação mais cuidadosa (que possivelmente não farei no primeiro momento), uma vez que, pela natureza do TCP, os segmentos que chegam à proxy já são frações do conteúdo integral e podem ser ainda mais fracionados em multiplos segmentos durante o roteamento ao peer seguinte. Portanto, é preciso garantir que todos os segmentos chegaram ao peer, ou enviar novamente os que encontraram erros. Acredito que a biblioteca `net` do Go já lide com isso e eu não preciso me preocupar, mas devo conferir essa informação.

> Vou começar a implementar e trago atualizações.

**Atualização (ddc73c7):** consegui implementar a proxy e parece estar funcionando bem. Ainda restaram alguns casos de erro que não estou lidando, vou deixar para polir isso no futuro. Felizmente, Go possui uma biblioteca nativa que contém uma função para conectar duas sockets (`io.Copy`), já lidando com eventuais erros, mas, por agora, quero implementar e melhorar minha própria versão para contribuir com o aprendizado.

**Atualização (f809262):** como esperado, a estrutura do balancer em si foi bastante simples. Vou apenas melhorar a implementação da proxy antes de escrever algumas estratégias do balancer.


## 03/04
Adicionei métodos básicos de balanceamento. Agora, para finalizar a primeira versão do projeto, faltam o arquivo de configuração e logs mais eficientes.
# Conclusões finais
O load balancer é HTTP compliant. Na realidade, é compliant com qualquer protocolo que utilize TCP. No entanto, não é possível acessar um host com HTTPS, visto que o acordo para chaves TLS funciona. Uma possibilidade pode ser intermediar inclusive a troca de chaves durante o TLS handshake, mas, ainda assim, o cliente não teria o certificado do servidor, apenas o da proxy, fazendo com que não seja possível confiar e estabelecer a conexão.

Assim, como querermos manter o endereço dos servidores em segredo, a melhor solução que me vem a mente é utilizar HTTPS tanto na conexão cliente-balancer quanto na balancer-server.

É importante notar que, dessa forma, a requisição estará completamente descriptografada para o load balancer, o que pode ser uma preocupação de privacidade a depender do modo como o sistema foi estruturado.

Visto que esse é um projeto de código aberto e quem está rodando provavelmente será a mesma pessoa que controla os servidores, a privacidade não é um problema (essa lógica pode ser generalizada para qualquer load balancer). No entanto, utilizar um load balancer (ou qualquer tipo de proxy) de terceiros pode ser preocupante caso implementem o modelo que descrevi.
