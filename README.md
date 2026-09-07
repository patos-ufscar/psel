# Sobre o Projeto

Este é o repositório do meu Load Balancer desenvolvido para o Processo Seletivo do Patos.

# Como usar
1. Clone o repositório:

```bash 
    git clone https://github.com/renan-michelao/psel-by-renan
```

2. Inicie o Load Balancer

```bash 
    cd psel-by-renan/load-balancer
    cargo run --bin load-balancer 
 ```

3. Inicie os Backends (Você deve abrir 3 terminais)

```bash
    # Terminal 1
    cargo run --bin backend

    # Terminal 2
    cargo run --bin backend2

    # Terminal 3
    cargo run --bin backend3
```

# Navegador

Abra o navegador na seguinte URL: 127.0.0.1/8080

# Como enviar arquivos (POST)

Abra o local onde seu arquivo está e faça:

```bash
    curl -X POST --data-binary @"seu_arquivo" http://127.0.0.1:8080/nome_arquivo
```

Depois é só fazer uma requisição para ver seu arquivo: 127.0.0.1:8080/nome_arquivo