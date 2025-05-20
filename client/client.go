package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
)

func main() {
	conn, err := net.Dial("tcp", "localhost:9000")
	if err != nil {
		fmt.Println("Erro ao conectar no proxy:", err)
		return
	}
	defer conn.Close()

	scanner := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("Digite uma mensagem: ")
		if !scanner.Scan() {
			break
		}
		msg := scanner.Text()

		_, err := conn.Write([]byte(msg + "\n"))
		if err != nil {
			fmt.Println("Erro ao enviar mensagem:", err)
			break
		}

		response, err := bufio.NewReader(conn).ReadString('\n')
		if err != nil {
			fmt.Println("Erro ao receber resposta:", err)
			break
		}
		fmt.Println("Mensagem recebida do servidor:", response)
	}
}