# PATOS PSEL

19 Agosto 2024 - ~~02 Setembro 2024~~ 09 Setembro 2024

A data final foi adiada uma semana, mas sinta-se livre para abir o PR quando quiser.

---

Dê um fork neste repositório e desenvolva nele. Quando estiver pronto, abra um PR.

Deve ser desenvolvido um `Reverse Proxy/Servidor de Arquivos` (a partir de `Berkley Sockets`) em sua linguagem de preferência, de qualquer forma, é preciso conseguir acessá-la de um navegador (HTTP).

[**sem lib dmais** -> PRECISA fazer o parsing do HTTP na mão].
Não é pra fazer assim:

```py
from thingdoer import ThingDoer

ThingDoer.do_thing()
```

Linguagens:

- C/C++
- ASM
- Go
- Rust
- Zig
- Clojure
- Erlang
- Qualquer outra desde que não tenha muita coisa pronta (ou seja, não para python ou JS)

(Nossa recomendação é algo tipo C++11 / Go, é bem parecido com C, mas deixa mais facil pq tem string, vector, map etc)

Lembrando que o objetivo não é ser algo extremamtente complexo, a ideia principal é integrar
direto com o S.O., ou seja, NÃO utilizar libs que abstraiam demais as interações. Queremos
saber o que vocês acharam difícil, como resolveram, onde acharam que poderiam ter ido mais
a fundo, etc. Quanto mais coisas "prontas" vocês utilizarem, menos coisa teremos para te avaliar, lembre-se disso.

As entregas serão individuais, mas sintam-se à vontade para trabalhar/discutir em grupo.

**O Código deve ser entregue por um repositório no GitHub, lembre-se de adicionar um README.md**

No final, haverá uma entrevista individual.

Pontos de Avaliação Essenciais:

- **HTTP compliant (conseguir acessar pelo navegador)**
- **Documentação**
- **Colaboração (documentar fontes de informação/código, informar sua jornada)**
- **Organização e Versionamento de Código**
- **Experiência num Geral, não apenas código**

Pontos de Avaliação Extras (faça oq conseguir/quiser, são apenas pontos para vocês se interessarem,
não precisam ser explorados):

- testes
- host validation
- max connections
- num workers
- tratamento de erros
- memory safety
- deploy (compilação, empacotamento)
- telemetria/SRE

Recursos extras:

- https://www.youtube.com/watch?v=iuwSYRdxKjQ
- https://beej.us/guide/bgnet/html/split/
- https://github.com/mr21/strsplit.c
- https://developer.mozilla.org/en-US/docs/Web/HTTP
- https://en.wikipedia.org/wiki/HTTP
- https://documentation.help/DogeTool-HTTP-Requests-vt/http_request.htm (ta zuado, mas as fts são boas)

Obs:

- Sinta-se à vontade para nos contactar a qualquer momento sobre qualquer dúvida, adoraremos ajudar!
- Se tiver uma dúvida geral, ou queira deixar sua dúvida pública utilize a área de [Issues](https://github.com/patos-ufscar/psel/issues)
- Não se sinta pressionado em fazer tudo, foque no que se sente confortável
- Envie mesmo se não conseguir terminar as partes essenciais, documente suas dificuldades
- Escreva um arquivo de texto/Markdown que descreva seu processo

> Lembrando que o processo é pra ser bem de boa, queremos ver até onde conseguem ir/se empurram, sem preocupação em fazer todos os essenciais.
