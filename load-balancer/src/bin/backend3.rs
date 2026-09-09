use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::fs;

fn extrai_caminho_arquivo(requisicao: &str) -> (String, String)   {
    if let Some(primeira_linha) = requisicao.lines().next() {

        // Divide a linha em pedaços usando espaços em branco
        let partes: Vec<&str> = primeira_linha.split_whitespace().collect();

        // garante que a requisição está bem formatada (pelo menos método e caminho)
        if partes.len() >= 2 {
            let metodo = partes[0].to_string();
            let caminho = partes[1];

            println!("Método: {}", metodo);
            println!("Caminho: {}", caminho);


            let caminho_formatado = if caminho == "/" {
                String::from("index.html")
            } else{
                caminho[1..].to_string()
            };

            return (metodo, caminho_formatado);

        }

    }

    // Retorno padrão caso a requisição esteja errada
    (String::from("GET"), String::from("404.html"))
}


fn extrai_tamanho_arquivo(requisicao: &str) -> usize {
    for linha in requisicao.lines() {
        if linha.to_lowercase().starts_with("content-length") {
            let pedacos: Vec<&str> = linha.split(":").collect();

            if pedacos.len() == 2 { // caso tenha um espaço de separação entre : e o número
                // Pega o número (ex: " 15042"), remove os espaços e converte para número (usize)
                let numero_str = pedacos[1].trim();
                if let Ok(tamanho) = numero_str.parse::<usize>() { // transforma o número de str para número usize
                    return tamanho;
                }
            }
        }
    }

    0   // se não achar o cabeçalho, assume que o arquivo tme 0 bytes
}

fn descobre_content_type(nome_arquivo: &str) -> &str {
    if nome_arquivo.ends_with(".html"){
        "text/html"
    } else if nome_arquivo.ends_with(".js") {
        "application/javascript"
    } else if nome_arquivo.ends_with(".wasm") {
        "application/wasm"
    } else if nome_arquivo.ends_with(".zip") {
        "application/zip"
    } else if nome_arquivo.ends_with(".png") || nome_arquivo.ends_with(".jpg") {
        "image/jpeg"
    } else if nome_arquivo.ends_with(".pdf") {
        "application/pdf"
    } else {
        "plain/text"
    }
}

fn backend_cabuloso(mut stream: TcpStream) {
    let mut buffer = [0; 4096];

    if let Ok(bytes_lidos) = stream.read(&mut buffer) {
        if bytes_lidos > 0{
            // Converte a requisição para um string
            let requisicao_str = String::from_utf8_lossy(&buffer[..bytes_lidos]);

            // Extrai qual arquivo o navegador está pedindo
            let (metodo, nome_do_arquivo) = extrai_caminho_arquivo(&requisicao_str);

            let caminho_no_disco = format!("public/{}",nome_do_arquivo);

            println!("Backend vai tentar ler: {}", caminho_no_disco);

            if metodo == "POST" {
                println!("Método POST, salvando arquivo: {}", caminho_no_disco);

                // O corpo do arquivo começa depois de dois \r\n seguidos
                let mut inicio_arquivo = 0;

                for i in 0..bytes_lidos.saturating_sub(3) {
                    if buffer[i] == b'\r' && buffer[i+1] == b'\n' && buffer[i+2] == b'\r' && buffer[i+3] == b'\n' {
                        inicio_arquivo = i + 4; // Pula os caracteres de quebra de linha
                        break;
                    }
                }

                // Pega os bytes do arquivo
                let mut bytes_do_arquivo = buffer[inicio_arquivo..bytes_lidos].to_vec();

                let tamanho = extrai_tamanho_arquivo(&requisicao_str);
                println!("tamanho do arquivo: {}", tamanho);

                if requisicao_str.to_lowercase().contains("expect: 100-continue") {
                    let _ = stream.write_all(b"HTTP/1.1 100 Continue\r\n\r\n");
                }

                while bytes_do_arquivo.len() < tamanho {
                    let mut buffer = [0; 4096];

                    match stream.read(&mut buffer) {
                        Ok(lidos) => {
                            if lidos == 0 {
                                break;
                            }

                            bytes_do_arquivo.extend_from_slice(&buffer[..lidos]);
                        }
                        Err(_) => break, // erro na rede
                    }
                }

                // tenta escrever no HD
                match fs::write(&caminho_no_disco, &bytes_do_arquivo){
                    Ok(_) => {
                        println!("Arquivo salvo, vamooooooo");
                        let corpo = "<h1>Arquivo salvo</h1>";
                        let resposta = format!("HTTP/1.1 201 Created\r\nContent-Length: {}\r\n\r\n{}", corpo.len(), corpo);
                        let _ = stream.write_all(resposta.as_bytes()); // manda a resposta para o cliente
                    }
                    Err(e) => {
                        println!("deu merda na hora de salvar o arquivo: {}", e);
                        let corpo = "<h1>deu red mano</h1>";
                        let resposta = format!("HTTP/1.1 500 deu Internal Server Error\r\nContent-Length: {}\r\n\r\n{}", corpo.len(), corpo);
                        let _ = stream.write_all(resposta.as_bytes());
                    }
                }
            } else if metodo == "GET" {

                match fs::read(&caminho_no_disco) {
                    Ok(conteudo_do_arquivo) => {
                        println!("Arquivo encontrado. manda o bglh pro load balancer");

                        let tamanho = conteudo_do_arquivo.len();

                        let tipo = descobre_content_type(&caminho_no_disco);

                        let cabecalho = format!("HTTP/1.1 200 OK\r\nContent-Type: {}\r\nContent-Length: {}\r\nConnection: close\r\n\r\n", tipo, tamanho);

                        let mut resposta_completa = cabecalho.as_bytes().to_vec();
                        resposta_completa.extend(conteudo_do_arquivo);

                        // Escreve tudo de volta no socket
                        if let Err(e) = stream.write_all(&resposta_completa) {
                            println!("Erro ao enviar a resposta: {}", e);
                        }

                    }
                    Err(_) => {
                        println!("Arquivo não encontrado. 404");

                        // Tenta ler o arquivo 404.html da pasta public
                        let conteudo_404 = fs::read("public/404.html").unwrap_or_else(|_| {
                            // Se não encontrar o arquivo, retornar essa string
                            String::from("<html><body><h1>404 - Arquivo 404.html ausente</h1></body></html>").into_bytes()
                        });
                        let cabecalho = format!(
                            "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: {}\r\nConnection: close\r\n\r\n", 
                            conteudo_404.len()
                        );
                        // Junta tudo
                        let mut resposta_erro = cabecalho.as_bytes().to_vec();
                        resposta_erro.extend(conteudo_404);

                        if let Err(e) = stream.write_all(&resposta_erro){
                            println!("Erro ao enviar erro 404: {}", e);
                        }
                    }
                }
            }
        }
    }
}

fn main() {
    let endereco = "127.0.0.1:8083";
    let listener = TcpListener::bind(endereco).expect("Erro na porta 8083");
    println!("Backend escutando em {}", endereco);

    for stream in listener.incoming() {
        if let Ok(stream) = stream {
            backend_cabuloso(stream);
        }
    }
}