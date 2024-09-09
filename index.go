package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"strings"
)

// Função para tratar a requisição HTTP
func handleConnection(conn net.Conn) {
	defer conn.Close()

	reader := bufio.NewReader(conn)
	requestLine, err := reader.ReadString('\n')
	if err != nil {
		fmt.Println("Erro ao ler a request line:", err)
		return
	}

	requestParts := strings.Fields(requestLine) // splita a primeira linha (método, url e versão http)
	if len(requestParts) != 3 {
		conn.Write([]byte("HTTP/1.1 400 Bad Request\r\n\r\n"))
		return
	}

	method := requestParts[0]
	urlPath := requestParts[1]

	urlPath = strings.TrimPrefix(urlPath, "/")
	pathParts := strings.SplitN(urlPath, "/", 2)

    // Improvisei um jeito de definir o sistema de arquivo por usuário, colocando o usuário no path da requisição (que servirá também como estrutura de pastas)
    // Por enquanto não possui um sistema de autenticação e provavelmente não é nada seguro KKKKKK
	if len(pathParts) != 2 {
		conn.Write([]byte("HTTP/1.1 400 Bad Request\r\n\r\n"))
		return
	}

	username := pathParts[0]
	fileName := pathParts[1]

    baseDir := "./arquivos"
    os.Mkdir(baseDir, 0755)
	userDir := fmt.Sprintf("%s/%s", baseDir, username)

	if _, err := os.Stat(userDir); os.IsNotExist(err) {
		err := os.Mkdir(userDir, 0755)
		if err != nil {
			conn.Write([]byte("HTTP/1.1 500 Internal Server Error\r\n\r\n"))
			return
		}
	}
    
    // Trata os métodos diferentes
	if method == "GET" {
		handleGetRequest(conn, userDir, fileName)
	} else if method == "POST" {
		handlePostRequest(conn, reader, userDir, fileName)
	} else {
		conn.Write([]byte("HTTP/1.1 405 Method Not Allowed\r\n\r\n"))
	}
}

func handleGetRequest(conn net.Conn, userDir string, fileName string) {
    defer conn.Close()
	filePath := fmt.Sprintf("%s/%s", userDir, fileName)

	file, err := os.Open(filePath)

    // Se ocorrer erros ao abrir o arquivo (não existir ou algum outro erro)
	if err != nil {
		if os.IsNotExist(err) {
			conn.Write([]byte("HTTP/1.1 404 Not Found\r\n\r\n"))
		} else {
			conn.Write([]byte("HTTP/1.1 500 Internal Server Error\r\n\r\n"))
		}
		return
	}
	defer file.Close()

	// Cabeçalho
	header := fmt.Sprintf("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Disposition: attachment; filename=%s\r\n\r\n", fileName)
    conn.Write([]byte(header))

	io.Copy(conn, file)
}

func handlePostRequest(conn net.Conn, reader *bufio.Reader, userDir string, fileName string) {
    defer conn.Close()
	filePath := fmt.Sprintf("%s/%s", userDir, fileName)

	file, err := os.Create(filePath)
	if err != nil {
		conn.Write([]byte("HTTP/1.1 500 Internal Server Error\r\n\r\n"))
		return
	}
	defer file.Close()

    // Cabeçalho
	for {
		line, err := reader.ReadString('\n')
		if err != nil {
			conn.Write([]byte("HTTP/1.1 400 Bad Request\r\n\r\n"))
			return
		}
        // Fim do cabeçalho
		if line == "\r\n" {
			break
		}
	}

	// Corpo do texto
	_, err = io.Copy(file, reader)
	if err != nil {
		conn.Write([]byte("HTTP/1.1 500 Internal Server Error\r\n\r\n"))
		return
	}

	conn.Write([]byte("HTTP/1.1 201 Created\r\n\r\n"))
	fmt.Fprintf(conn, "Arquivo '%s' salvo com sucesso", fileName)
}

func main() {
    // Escuta na porta 8080
	listener, err := net.Listen("tcp", "localhost:8080")
	if err != nil {
		log.Fatalf("Erro ao iniciar o servidor: %v", err)
	}
	defer listener.Close()

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Println("Erro ao aceitar conexão:", err)
			continue
		}

		go handleConnection(conn)
	}
}
