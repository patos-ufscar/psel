# PATOS PSEL

O código desenvolvido é uma sistema de arquivos simples com um proxy reverso para interceptar as solicitações. O sistema funciona da seguinte forma:
- Cria um listener para ficar escutando na porta 8080
- Ao receber uma requisição, lê o cabeçalho para saber como tratá-la
- Verifica o método: se for POST adiciona um arquivo, se for GET lê o arquivo do caminho informado
  
**OBSERVAÇÃO:** A requisição deve ser feita para um diretório (simulando o diretório de arquivos de um usuário) e para um nome de arquivo, por exemplo:

```
POST /pedro/senhas.txt HTTP/1.1  
Host: localhost:8080  
Content-Type: text/plain

senhanadavulneravel
```
Para enviar arquivos deve-se envia-lo em binário, por exemplo:
```
curl -X POST http://localhost:8080/pedro/arquivo.exe --data-binary @arquivo.exe -H "Content-Type: application/octet-stream"
```

### Dificuldades  
- Já conhecia um pouco de sockets por conta de algumas disciplinas como SD, então minha maior dificuldade foi programar em uma linguagem que nunca tinha mexido (e entender oq eu tava fazendo kk)  
- Estranhei um pouco também as funções para tratar as strings e as libs de conexão, mas depois de um tempo foi fácil de entender

### Materiais
- https://pkg.go.dev/strings
- https://pkg.go.dev/net
- GPT quando eu tava travado em algum ponto
