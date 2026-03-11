# Servidor HTTP / Servidor de pastas em C++ - PATOS PSEL

## Sobre

Este projeto foi desenvolvido para o processo seletivo do PATOS. O objetivo foi criar um Servidor de pastas / Reverse Proxy para processar pedidos HTTP de forma nativa via navegador, utilizando a API de **Berkeley Sockets**.

O projeto foi feito em **C++** com uso de bibliotecas padrão (como `std::string`, `std::vector` e `std::map`) para a manipulação de dados e parsing de texto.

## Funcionalidades e Requisitos

### Essenciais
- **HTTP Compliant:** O servidor pode ser acessado a partir de qualquer navegador web padrão (Chrome, Edge, Firefox)
- **Parsing Manual:** Todo o processamento do protocolo HTTP (métodos, cabeçalhos, corpo) foi desenvolvido manualmente a partir dos buffers recebidos pelo socket
- **Servidor de Pastas Estáticos:** Mapeamento de rotas da URL para a leitura de pastas no sistema (HTML, CSS, JS)

### Extras
- **Multi-threading (Workers):** Utilização da biblioteca `<thread>` para lidar com múltiplas conexões em simultâneo. O servidor não bloqueia à espera de finalizar o envio de uma pasta para atender um novo cliente
- **Tratamento de Segurança Mínimo (Directory Traversal):** Implementação de uma camada de segurança simples que impede atacantes de utilizarem `../` na URL para acessar aas pastas confidenciais fora do *Document Root* (`/www`).
- **Tratamento de Erros:** O servidor devolve *Status Codes* HTTP como `200 OK` para pastas encontradas, `404 Not Found` para recursos inexistentes e `403 Forbidden` para tentativas de acesso indevido

## Arquitetura e Estrutura de Pastas

O projeto foi refatorado e dividido em pastas para manter a organização do código:

- `src/main.cpp`: Ponto de entrada da aplicação, instancia e inicia o servidor
- `src/ServerSocket.cpp / .h`: Encapsula a API do sistema operacional (`getaddrinfo`, `socket`, `bind`, `listen`, `accept`)
- `src/HttpRequest.cpp / .h`: Estrutura de dados e lógica de parsing para transformar o texto bruto do socket num objeto manipulável
- `src/HttpResponse.cpp / .h`: Estrutura que formata os dados (código de estado, headers, body) numa string HTTP compliant
- `src/FileHandler.cpp / .h`: Cuida do *Document Root* (a pasta `/www`), faz a leitura das pastas e atribui as extensões corretas

## Como Compilar e Executar

1. **Clone o repositório:**
   ```bash
   git clone <esse fork>
   cd psel-patos/src
    ```

2. **Compile o projeto:**
    O projeto requer o standard C++11 (ou superior) e a biblioteca pthread para o suporte a multi-threading
     ```bash
    g++ -std=c++11 *.cpp -o server -pthread
     ```

3. **Inicie o servidor:**
     ```bash
    ./server
     ```


## Dificuldades e Decisões de Desenvolvimento

1. Bloqueio de I/O: Inicialmente eu fiz o servidor atendendo um cliente de cada vez. Adicionei a `std::thread` para que a simulação de conexões simultâneas funcionasse (quando uma página HTML solicita também a pasta CSS e uma pasta de imagem ao mesmo tempo, por exemplo).

2. Entender o problema: Tive certa dificuldade inicial de entender o conceito de reverse proxy, isso tornou um pouco complexo ao pensar no que fazer e em como fazer.

## Fontes de busca
- https://github.com/rhymu8354/SocketTutorial/tree/main
- https://www.dei.isep.ipp.pt/~asc/doc/sockets-berkeley.html (explicação MUITO boa)
- https://beej.us/guide/bgnet/html/split/