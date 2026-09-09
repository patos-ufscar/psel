# Journey 0: Inciando

Eu decidi fazer o projeto do Load Balacener em Rust. Mas antes de começar, eu preciso aprender o básico da sintaxe do Rust. Então eu crie a pasta "rust_basic" com alguns exercícios básicos em Rust. (Não sei se precisa enviar junto com o projeto, mas por via de duvidas irei fazer o commit desses exercícios também)


# Journey 1: O que é um Load Balancer?

O Load Balancer é um método de distribuir o tráfego de rede para o backend. Ele repassa as requisições de forma a garantir que nenhum servidor fique sobrecarregado enquanto outro fica vago.

Vou usar o algoritmo Round-Robin para fazer o roteameto:

# Proxy Reverso

Um servidor web comum recebe uma mensagem e devolve um arquivo. O Load Balancer funciona como um proxy reverso, ou seja, é um servidor intermediário que fica entre o usuário/cliente e o servidor web. O Load Balancer atua como servidor para o usuário e como cliente para o backend.
       
Usuário -> Load Balancer (proxy reverso) -> Servidor Web

# Protocolo HTTP

Um pacote HTTP é estruturalmente apenas um bloco de texto padronizado. É dividido basicamente entre as seguintes partes:

*Request Line:* A primeira linha, contendo o Método, a Rota e a Versão. (Ex: GET /imagens/foto.png HTTP/1.1\r\n).

*Headers:* Linhas seguindo o formato *Chave: Valor*. (Ex: Host: localhost:8080\r\n).

*Body:* O conteúdo do pacote, usado quando fazemos requisições POST para enviar um arquivo.

OBS: A sequência de caracteres *\r\n\r\n* indica que o cabeçalho acabou.


# Journey 2: Iniciando o backend

Comecei a fazer o backend que será o servidor de arquivos. Por enquanto, o backend está extremamente simples, apenas retorna
um HTML simples de teste. 

Bom, até o momento deste commit, o que eu tenho é um esqueleto de um load balancer (ainda não é um load balancer e nem funciona como tal, mas chegaremos lá)
rodando na porta 8080, onde recebe uma requisição e a repassa para o backend, que está rodando na porta 8081. O backend confirma que recebeu a requisição e retorna um HTML para o socket, devolvendo a resposta para o load balancer, e o load balancer devolve a resposta para o cliente (é possível ver a resposta no navegador).

Eu tive um problema para conectar o load balancer com o backend, e depois de um tempo analisando o código e pensando no que estava errado, eu desisti de procurar e apelei para a IA. E pasmem, eram apenas dois erros simples: um erro de sintaxe (ainda não estou habituado com o Rust) e o outro era que eu estava usando a variável errada (falta de atenção que me custou pelo menos 15 minutos).

Também tive dificuldade com a sintaxe que usei para ler a resposta do backend, onde 'let bytes_resposta' = match... Esse operador de controle de fluxo é muito bom, mas ainda não me acostumei muito com ele hahaha. Tive que escrever e reescrever esse bloco de código algumas vezes até conseguir fazer funcionar e compreender como ele funciona.

# Dia 2 - Backend

Fiz algumas melhorias no backend:

+ "extrai_caminho_arquivo": É uma função que pega a primeira linha da requisição e a divide em duas partes. A primeira parte é o método, onde extraimos o método usado na requisição (como a requisição está sempre pedindo um arquivo, o método será sempre GET). A segunda parte é o caminho efetivo do arquivo.

+ "backend_cabuloso": Essa função é responsável por manipular o arquivo que foi requisitado, ou seja, a função vai tentar abrir o arquivo. Caso o arquivo seja encontrado, a função vai escrever todo o conteúdo do arquivo de volta para socket, ou seja, vai mandar o conteúdo do arquivo de volta para o cliente. 
> Preciso dar um nome melhor para essa função 

Tenho dois arquivos para testes: Ao rodar 127.0.0.1:8080 (caminho padrão), vai ser retornado o arquivo "index.html". No caminho 127.0.0.1:8080/imagem.jpg, é retornado a logo do patos (que por algum motivo desconhecido aparece cortada no navegador).

Well, acho que por enquanto é isso. Ainda não temos um Load Balancer de fato, é apenas um proxy reverso (por enquanto), vamos chegar lá.


# Journey 3 - Threads e Round-Robin

Fiz alterações importantes no Load Balancer. A primeira coisa é que ele deixou de ser apenas um proxy reverso e agora atua como um Load Balancer de verdade. Usei a biblioteca de Threads para poder servir os backends em pararelo, então se um usuário está usando o servidor na porta 8081 e chega outro usuário, o novo usuário é direcionado para o servidor na porta 8082 e se chega outro usuário, ele é redirecionado para o servidor na porta 8083. E para fazer este direcionamento de usuários para os servidores, eu utilizei o algoritmo Round-Robin, que basicamente faz o balanciamento de carga distribuindo as requisições de maneira sequencial e quando acaba os servidores disponíveis, ele volta o contador para zero e começa o balanciamento novamente (acho que ficou um pouco confuso essa explicação).

Ex: Eu tenho uma lista com 3 servidores disponíveis, o algorítmo Round Robin usa um contador que recebe os índices da lista e direciona o primeiro usuário para o primeiro servidor da lista, o segundo usuário para o segundo servidor da lista e o terceiro usuário para o terceiro servidor da lista. Quando o contador for igual ao tamanho da lista, ou seja, quando acabar os servidores disponíveis, o contador vai reiniciar e vai começar a balancear as requisições a partir do primeiro servidor da lista novamente.

Para testar esse balanceamento entre os servidores, eu criei mais dois backends que funcionam como servidores de teste, e com isso é possível ver nas suas saídas no terminal, que eles recebem uma nova requisição de acordo com o contador do Round Robin. 

Para conseguir visualizar melhor, basta rodar o Load Balancer e os três servidores juntos, ao acessar o Load Balancer na porta 8080 e ficar recarregando a página, é possível ver o balanceamento acontecendo no terminal, onde a porta que está recebendo a nova requisição está mudando a cada refresh na página.

# Método GET - funcionando
Descobri o motivo da imagem de teste aparecer cortada no navegador: O buffer que armazena a resposta do backend tem apenas 4KB e quando o buffer enche, o resto da informação fica perdida/não chega no buffer. Para resolver isso, eu fiz um loop para encher o buffer de 4KB em 4KB e manda para o cliente. Quando o "bytes_lidos" é zero, significa que não tem mais nada para ler, ou seja, toda informação já foi passada do buffer para o cliente. Com isso a imagem não fica mais cortada.


# Método POST

Decidi implementar o método POST para salvar arquivos no servidor também. Então eu implementei uma lógica simples para separar o método GET e POST no backend usando if/else. Eu extraio o método da requisição usando a função "extrai_caminho_arquivo" e o retorno dessa função é o método e o caminho da requisição. Ai na função "backend_cabuloso" (não, ainda não mudei o nome) ele vai fazer um if para verificar se é um método POST ou GET e a partir daí vai processar a requisição de acordo com o seu respectivo método. Mas obviamente nem tudo são flores e já encontrei um problema: é basicamente o mesmo problema que eu tive no buffer para o método GET, ou seja, o buffer de resposta enche os seus 4KB e fecha a conexão e todo o resto da informação fica perdida. E pelo o que eu pesquisei, não é possível resolver este problema usando um loop simples como eu fiz no método GET. Então vou pesquisar mais para saber como resolver isso da meneira correta. 

# Dia 2 - Método POST

Implementei uma função para extrair o tamanho do arquivo que está sendo salvo. Ela basicamente pega a requisição e procura a linha "Content-Length: ....." e retorna esse número (que é o tamanho do arquivo). Fiz isso para o POST conseguir fazer o upload do arquivo completo usando um While, ou seja, enquando ele não ler todos os bytes do arquivo que sendo salvo, ele continua lendo.

Percebi que o POST não estava salvando os arquivos. Nos primeiros teste que fiz estava salvando normal, mas eu tirei algumas coisas que eu achei que não estavam influenciando em nada no código e subi pro GIT, mas o POST parou de funcionar.

Como eu estou testando o UPLOAD de arquivos com o "curl", ele pede uma permissão para o POST, e como eu não fornecia essa permissão, ele não salva o arquivo. Portanto, adicionei uma condição que, caso o curl peça uma permissão, o código manda um "Continue" para o HTTP. E aparentemente os arquivos estão sendo salvos corretamentes.

Eu dei uma olhada nesses links para fazer tudo isso até o momento (teve outro site, mas não lembro qual foi) e vi alguns vídeos no Youtube sobre como os Load Balancers funcionam. O resto fui caminhando com o auxilio da IA.

fontes - POST:
https://developer.mozilla.org/pt-BR/docs/Web/HTTP/Reference/Methods/POST
https://medium.com/@gabriellamedas/the-http-server-explained-c41380307917
https://aws.amazon.com/what-is/load-balancing/


# Journey 4 - Content-Type

Eu estava tendo um problema que nem sempre os arquivos carregavam no GET, por isso eu criei uma função para pegar o tipo do arquivo e, então esse problema foi resolvido. Todos os arquivos que eu testei, ele conseguiu retornar sem problemas. Criei essa função também para a próxima jornada.

# Journey 5 - Rodar Doom

Pesquisei um pouco sobre como rodar Doom na web e decidi usar a engine do JS-DOS, para emular o sistema DOS direto no navegador usando WebAssembly. Ao pedir "doom.html" no Load Balancer, ele pede os outros arquivos necessários. O JS-DOS compila o WebAssembly, descompacta o "doom.zip" e executa o "doom.exe" direto no emulador DOS rodando no navegador.

Utilizei os seguintes arquivos:

curl -O https://js-dos.com/6.22/current/js-dos.js
curl -O https://js-dos.com/6.22/current/wdosbox.js
curl -O https://js-dos.com/6.22/current/wdosbox.wasm
curl -O https://js-dos.com/6.22/current/wdosbox.wasm.js
curl -L -o doom.zip "https://archive.org/download/DoomsharewareEpisode/doom.ZIP"

# Considerações Finais

Well, acho que finalmente terminei este projeto de Load Balancer. Posso dizer que foi um desafio bem desafiador, ainda mais para mim que não tinha conhecimento algum sobre redes hahaha. Mas foi divertido, e claro, é gratificando e bem legal ver o projeto funcionando na prática. Eu ainda não entendi a fundo como as coisas da rede funcionam (sou meio ateu nisso ainda), espero ter realmente aprendido o básico de como isso funciona (vamos descobrir na entrevista). Enfim, foi bem legal desenvolver isso, aprendi algumas coisas, na qual eu nem fazia idéia que existiam e eram tão necessárias. Enfim, dale Patos.