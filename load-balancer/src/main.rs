use std::io::{Read, Write};

// Importa os componentes de rede 
use std::net::{TcpListener, TcpStream};

use std::thread;
use std::sync::{Arc, Mutex}; // componentes para lidar com memória compartilhada entre threads

fn processa_clinte(client_stream: TcpStream, endereco_backend: String){
    println!("---- Nova Conexão ----");
    println!("Roteando para o backend: {}", endereco_backend);

    // Conecta ao servidor backend
    let backend_stream = match TcpStream::connect(&endereco_backend) {
        Ok(stream) => stream,
        Err(e) => {
            println!("Erro ao conectar no backend {}: {}", endereco_backend, e);
            return;
        }
    };

    // clona as conexões, permitindo leitura e escrita ao mesmo tempo de forma independente
    let mut cliente_leitura = client_stream.try_clone().unwrap();
    let mut cliente_escrita = client_stream.try_clone().unwrap();
    
    let mut backend_leitura = backend_stream.try_clone().unwrap();
    let mut backend_escrita = backend_stream.try_clone().unwrap();

    // Rota de uploads (Cliente -> Backend)
    // cria uma thread só para mandar os dados do cliente para o backend
    let thread_upload = thread::spawn(move || {
        let mut buffer_ida = [0; 4096];
        loop {
            match cliente_leitura.read(&mut buffer_ida) {
                Ok(bytes_lidos) => {
                    if bytes_lidos == 0 {
                        break;
                    } // Cliente parou de enviar
                    
                    // Manda os bytes pro backend
                    if backend_escrita.write_all(&buffer_ida[..bytes_lidos]).is_err() {
                        break; 
                    }
                }
                Err(_) => {
                    break;
                }, // Erro na rede, quebra o loop
            }
        }
    });

    // Rota de downloads (Backend -> Cliente)
    // Usa a thread principal para mandar os dados do backend de volta para o cliente
    let mut buffer_volta = [0; 4096];
    loop {
        match backend_leitura.read(&mut buffer_volta) {
            Ok(bytes_lidos) => {
                if bytes_lidos == 0 { // Backend terminou de responder
                    break;
                } 
                
                // Manda os bytes pro cliente
                if cliente_escrita.write_all(&buffer_volta[..bytes_lidos]).is_err() {
                    break;
                }
            }
            Err(_) => break,
        }
    }

    // Espera o upload terminar antes de matar a função e fechar os sockets
    let _ = thread_upload.join();
    
    println!("Transferência concluída");
}

fn main(){

    // Lista com os servidores backend
    let lista_backends = vec![
        String::from("127.0.0.1:8081"),
        String::from("127.0.0.1:8082"),
        String::from("127.0.0.1:8083"),
    ];

    let contador_round_robin = Arc::new(Mutex::new(0));

    // Define o endereço e a porta que o load balancer vai escutar
    let endereco = "127.0.0.1:8080";
    let listener = TcpListener::bind(endereco).expect("Erro na porta 8080");

    println!("Load Balancer escutando em {}", endereco);

    // loop infinito que mantem o servidor rodando, esperando conexão
    for stream in listener.incoming(){
        match stream{
            Ok(stream) => {
                println!("Nova conexão");

                // Faz uma cópia das variáveis para jogar dentro da thread
                let contador_clone = Arc::clone(&contador_round_robin);
                let backends_clone = lista_backends.clone();

                thread::spawn(move || {
                    // Trava o mutex
                    let mut indice_atual = contador_clone.lock().unwrap();
                    let backend_escolhido = backends_clone[*indice_atual].clone();

                    *indice_atual = *indice_atual + 1;

                    // Se passou do tamanho da lista -> volta pra zero (round robin)
                    if *indice_atual >= backends_clone.len(){
                        *indice_atual = 0;
                    }

                    // Tira a trava do mutex
                    drop(indice_atual);

                    processa_clinte(stream, backend_escolhido);
                });
            }
            Err(e) => {
                println!("Erro ao aceitar conexão: {}", e);
            }
        }
    }
}