package main

import (
	"bufio"
	"fmt"
	"io"
	"mime"
	"net"
	"os"
	"path/filepath"
	"strings"
)

const (
	PORT            = "8080"
	FILE_SERVER_DIR = "./files"
)

/*
Cria um listenet TCP na porta 8080
Para cada conexão aceita, cria uma goroutine para processar a requisição
*/
func main() {
	listener, err := net.Listen("tcp", ":"+PORT)
	if err != nil {
		fmt.Println("Erro ao iniciar o servidor:", err)
		return
	}
	defer listener.Close()

	fmt.Println("Servidor rodando na porta", PORT)

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("Erro ao aceitar conexão:", err)
			continue
		}
		go handleConnection(conn)
	}
}

/*
Lê a requisição do client e processa o caminho do arquivo solicitado
Se o caminho for "/", o arquivo test.html será retornado
Se o arquivo não for encontrado, retorna um erro 404
Nessa função aqui eu passei sufoco
*/
func handleConnection(conn net.Conn) {
	defer conn.Close()

	reader := bufio.NewReader(conn)
	requestLine, err := reader.ReadString('\n')
	if err != nil {
		fmt.Println("Erro ao ler a requisição:", err)
		return
	}

	// Divide a linha da requisição em partes
	parts := strings.Split(strings.TrimSpace(requestLine), " ")
	if len(parts) != 3 {
		fmt.Println("Requisição inválida")
		return
	}

	// método, caminho, versão HTTP
	method, path, _ := parts[0], parts[1], parts[2]

	// Lê e ignora os headers
	for {
		line, err := reader.ReadString('\n')
		if err != nil || line == "\r\n" {
			break
		}
	}

	if method != "GET" {
		sendResponse(conn, 405, "Method Not Allowed", "")
		return
	}

	handleFileServer(conn, path)
}

// Lê o arquivo correspondente ao caminho e envia o conteúdo para o client
func handleFileServer(conn net.Conn, path string) {
	if path == "/" {
		path = "/test.html"
	}

	filePath := filepath.Join(FILE_SERVER_DIR, filepath.Clean(path))
	file, err := os.Open(filePath)
	if err != nil {
		sendResponse(conn, 404, "Not Found", "File not found")
		return
	}
	defer file.Close()

	fileInfo, err := file.Stat()
	if err != nil {
		sendResponse(conn, 500, "Internal Server Error", "")
		return
	}

	contentType := mime.TypeByExtension(filepath.Ext(filePath))
	if contentType == "" {
		contentType = "application/octet-stream"
	}

	// Envia o cabeçalho HTTP com o tipo de conteúdo e o tamanho do arquivo
	header := fmt.Sprintf("HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",
		contentType, fileInfo.Size())
	conn.Write([]byte(header))

	io.Copy(conn, file)
}

// Envia a resposta HTTP com o status, texto do status e corpo da resposta
func sendResponse(conn net.Conn, status int, statusText, body string) {
	response := fmt.Sprintf("HTTP/1.1 %d %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",
		status, statusText, len(body), body)
	conn.Write([]byte(response))
}
