package main

import (
	"bufio"
	"fmt"
	"net"
	"strings"
)

func main() {
	ln, err := net.Listen("tcp", ":9001")
	if err != nil {
		fmt.Println("Erro ao iniciar o servidor:", err)
		return
	}
	defer ln.Close()

	fmt.Println("Servidor esperando conexão do proxy...")
	conn, err := ln.Accept()
	if err != nil {
		fmt.Println("Erro ao aceitar conexão do proxy:", err)
		return
	}
	defer conn.Close()
	fmt.Println("Proxy conectado ao servidor")

	reader := bufio.NewReader(conn)
	for {
		msg, err := reader.ReadString('\n')
		if err != nil {
			fmt.Println("Erro ao ler mensagem do proxy:", err)
			break
		}
		fmt.Println("Mensagem recebida do proxy:", msg)

		if strings.TrimSpace(msg) == "exit" {
			fmt.Println("Mensagem de saída recebida. Encerrando a conexão...")
			break
		}

		_, err = conn.Write([]byte(msg))
		if err != nil {
			fmt.Println("Erro ao enviar resposta ao proxy:", err)
			break
		}
	}
}
