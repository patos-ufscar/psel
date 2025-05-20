# Reverse Proxy com Alerta no Telegram

This reverse proxy monitors incoming and outgoing connections, sending real-time notifications to a Telegram bot whenever a connection is established or closed. It also logs the exchanged messages, providing visibility into network traffic.

Este projeto é uma implementação simples de um **Reverse Proxy** com notificações de alertas via **Telegram** para notificar eventos importantes, como novas conexões e erros, além de receber um log das mensagens trocadas durante a conexão. Ele consiste em três componentes principais: um **servidor**, um **proxy** e um **cliente**.

## Tecnologias Utilizadas

- **Go (Golang)**: Linguagem de programação utilizada para o desenvolvimento do servidor, proxy e cliente.
- **Telegram API**: Utilizada para enviar notificações sobre eventos no proxy, como novas conexões, mensagens, erros e desconexões.
- **Net/TCP**: Biblioteca nativa do Go para comunicação via TCP entre os componentes do sistema.

## Descrição e Funcionamento  

Este projeto consiste em um **reverse proxy** que intermedia a comunicação entre um cliente e um servidor. Ele é composto por três componentes principais:  

1. **Servidor (`server/server.go`)**: Escuta na porta `9001` e aguarda conexões do proxy, respondendo às mensagens recebidas.  
2. **Proxy (`proxy/reverse_proxy.go`)**: Atua como intermediário, escutando a porta `9000`, recebendo mensagens do cliente e encaminhando-as ao servidor. Além disso, monitora conexões e falhas, enviando notificações para o Telegram.  
3. **Cliente (`client/client.go`)**: Conecta-se ao proxy, envia mensagens e aguarda uma resposta do servidor.  

### Fluxo de Comunicação  

- O **cliente** envia uma mensagem ao **proxy**, que a repassa para o **servidor**.  
- O **servidor** processa a mensagem e responde ao **proxy**, que a retorna ao **cliente**.  
- Sempre que uma nova conexão é estabelecida ou um erro ocorre, o **proxy** envia uma notificação para o **Telegram**, e assim que uma conexão é encerrada outro alerta é enviado junto com as mensagens que foram trocadas durante a conexão.
- No mesmo diretório do `reverse_proxy.go` um arquivo `mensagens.txt`é criado contendo as todas as mensagens de todas as conexões estabelecidas.


## Antes de mais nada
Primeiramente, você deve clonar esse repositório dentro do diretório que deseja trabalhar:
  ```bash
  git clone https://github.com/Triiltz/reverse-proxy-telegram.git
  ```

  Agora, antes de seguirmos para o código em si, primeiro precisamos criar o seu bot no Telegram que ficará responsável por receber as notificações do proxy. Para isso, você precisará criar um novo bot!

  ### Criando o seu bot no Telegram
  Para criar o seu bot no Telegram, você precisará seguir os passos abaixo:
  1. Abrindo o Telegram (tanto web quanto mobile) procure por `@BotFather` e comece uma conversa com `/start`;
  2. O `BotFather` irá lhe mostrar uma lista de opções, escolha `/newbot`;
  3. Agora é só ir seguindo os passos do chat, escolhendo um nome e username para ele, apenas sigue as instruções;
  4. No final você receberá uma mensagem de confirmação com o token do seu bot como algo do tipo "**Use this token to access the HTTP API: YOUR_TOKEN**", anote-o pois ele será necessário para o código.
  
  O próximo passo é conseguir o seu chat ID do telegram. Para isso, siga os passos:
  1. Procure pelo username do seu bot no Telegram e envie qualquer mensagem, apenas para iniciar a conversa;
  2. Agora, acesse a URL `https://api.telegram.org/botXXXXX:YYYYY/getUpdates` (substitua `XXXXX:YYYYY` pelo token do seu bot) que você salvou logo acima;
  3. Você verá um json parecido com esse:
   ```json
    {
  "ok": true,
  "result": [
    {
      "update_id": 4xxxxxxx5,
      "message": {
        "message_id": xx,
        "from": {
          "id": 6xxxxxxxx7,
          "is_bot": false,
          "first_name": "...",
          "last_name": "...",
          "username": "...",
          "language_code": "en"
        },
        "chat": {
          "id": 88xxxxxx99,
          "first_name": "...",
          "last_name": "...",
          "username": "...",
          "type": "private"
        },
        "date": xxxxxxxx,
        "text": "SUA_MENSAGEM_ENVIADA"
      }
    }
  ]
}
```

Nesse json verifique o valor de `result.0.message.chat.id`, esse é o seu chat ID, anote pois iremos utilizá-lo mais a frente (no exemplo acima, `"id": 88xxxxxx99`).
**OBS:** Caso esteja tendo problemas o acessar a URL e fique recebendo um json como

```json
  {
  "ok": true,
  "result": [] 
  } 
``` 
tente encerrar e iniciar o chat novamente, pois o chat ID é gerado apenas quando você iniciar uma conversa com o seu bot.


Ufa! Agora estamos prontos para ir ao código em si! Dentro do arquivo `reverse_proxy.go` na pasta `proxy`, você deve configurar o seu token do Telegram bot e o seu chat ID no seguinte trecho de código:
``` go
const (
	telegramToken = "BOT_TOKEN"	// Token do bot Telegram
	chatID        = "YOUR_CHAT_ID"	// ID do chat do Telegram
)
```
Substitua `BOT_TOKEN` pelo token do seu Telegram bot e `YOUR_CHAT_ID` pelo seu chat ID, ambos obtidos anteriormente

# Como executar
Para executar é muito simples, basta executar os três arquivos separadamente no seu terminal. SIga a sequência logo abaixo:

1. **Iniciando o servidor**:
    - Vá até a pasta `server` e execute o comando:
      ```bash
      go run server.go
      ```

2. **Iniciando o reverse proxy**:
    - Vá até a pasta `reverse_proxy` e execute o comando:
      ```bash
      go run reverse_proxy.go
      ```
3. **Iniciando o cliente**:
    - Vá até a pasta `client` e execute o comando:
      ```bash
      go run client.go
      ```

**OBS:**
  - Para utilizar o comando go run, é necessário ter o Go instalado na sua máquina! Caso não tenha, você pode baixar na [página oficial do Go](https://golang.org/dl)
  - O servidor e o proxy devem estar em execução antes de iniciar o cliente.


## Dificuldades Encontradas

Como todo projeto novo iniciado, enfrentei algumas dificuldades ao longo do caminho. Aqui estão algumas delas:

- **Barreira da linguagem**: Bom, eu basicamente tive que aprender Go para realizar esse projeto, sempre tive vontade de começar a de fato utilizar a linguagem, mas eu sempre procrastinava e ficava sem ideias/vontade, até que pensei "por que não unir o aprendizado com o projeto?". E foi isso que eu fiz! Confesso que de uma maneira mais rápida e objetiva, focando em aprender os fundamentos necessários para o projeto, mas passando por diversas fases importantes para o aprendizado da linguagem.

- **Problemas na automatização da execução**: Tentei criar um script em bash para automatizar a execução dos arquivos, porém, tive diversos erros, entre eles erros como falha na sincronização dos processos principalmente entre a conexão do server com o reverse_proxy devido algum problema do qual não conseguir resolver (e não foi por falta de tentativas). Outro problema envolvendo a automatização foi por conta do encerramento dos processos nas portas utilizadas, somente depois eu consegui resolver isso no script matando os processos depois da execução;

- **Codificação e Formatação de Mensagens no Telegram**: Ao implementar a função sendTelegramMessage, dei de cara com um problema que até então não sabia que era um problema: caracteres especiais e formatação HTML eram corrompidos ao enviar mensagens para a API do Telegram.
    - **Solução**: Descobri que precisava usar url.Values{} para codificar corretamente os dados no formato x-www-form-urlencoded, garantindo que:
      - Emojis, acentos e símbolos (como & ou <) não quebrassem a requisição.
      - Tags HTML (como `<b>` ou `<pre>`) fossem preservadas quando combinadas com parse_mode=HTML. Sem isso, as mensagens chegavam truncadas ou sem formatação — um detalhe muito importante que me consumiu um bom tempo até ser entendido e corrigido.

## Conclusão e próximos passos

Este projeto é uma boa demonstração de como um reverse proxy pode ser implementado utilizando Go, mas também destaca as complexidades de se trabalhar com comunicação de rede, uso de uma API externa e o teste de automação de processos. Além disso, foi uma ótima oportunidade para aprender sobre a linguagem Go e ver o potencial incrível que ela possui e as suas diversas funcionalidades e flexibilidades! Espero dar continuidade a este projeto, como por exemplo Dockerizar para fazê-lo rodar em um ambiente isolado e de fácil portabilidade e, futuramente, implementar mais funcionalidades, melhorias e rever alguns conceitos que podem vir a mudar de alguma forma.  

### Referências 
  - https://www.youtube.com/watch?v=iuwSYRdxKjQ (pra dar uma relembrada em alguns conceitos);
  - https://developer.mozilla.org/en-US/docs/Web/HTTP;
  - https://gist.github.com/nafiesl/4ad622f344cd1dc3bb1ecbe468ff9f8a (consulta de problemas sobre o meu chat ID do Telegram);
  - https://okanexe.medium.com/the-complete-guide-to-tcp-ip-connections-in-golang-1216dae27b5a (isso me ajudou demais);
  - https://go.dev/src/net/example_test.go;



## License

Este projeto está licenciado sob a MIT License - veja o arquivo [LICENSE](LICENSE) para mais detalhes.
