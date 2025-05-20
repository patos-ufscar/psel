package main

import (
	"bufio"
	"fmt"
	"net"
	"net/http"
	"net/url"
	"os"
	"strings"
	"time"
)

const (
	telegramToken = "7913053052:AAFYobTmDYMRDcSJfGnwR5PJ2nkAuD4hKMc" // token do telegram bot
	chatID        = "1006182052"                                     // ID do seu chat pessoal do Telegram
)

func sendTelegramMessage(message string) {
	client := &http.Client{}

	// cria o payload como form-data (evita problemas com URL encoding)
	form := url.Values{}
	form.Add("chat_id", chatID)
	form.Add("text", message)
	form.Add("parse_mode", "HTML") // permite formatacao basica

	req, err := http.NewRequest(
		"POST",
		fmt.Sprintf("https://api.telegram.org/bot%s/sendMessage", telegramToken),
		strings.NewReader(form.Encode()),
	)
	if err != nil {
		fmt.Println("Erro ao criar requisição:", err)
		return
	}

	req.Header.Add("Content-Type", "application/x-www-form-urlencoded")

	resp, err := client.Do(req)
	if err != nil {
		fmt.Println("Erro ao enviar para o Telegram:", err)
		return
	}
	defer resp.Body.Close()

	fmt.Println("Status do Telegram:", resp.Status)
}

func main() {
	// cria o arquivo que ira armazenar as mensagens
	logFile, err := os.OpenFile("mensagens.txt", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		fmt.Println("Erro ao criar arquivo de log:", err)
		return
	}
	defer logFile.Close()

	ln, err := net.Listen("tcp", ":9000")
	if err != nil {
		fmt.Println("Erro ao iniciar o proxy:", err)
		return
	}
	defer ln.Close()

	for {
		fmt.Println("\nProxy esperando conexão do cliente...")
		clientConn, err := ln.Accept()
		if err != nil {
			fmt.Println("Erro ao aceitar conexão do cliente:", err)
			continue
		}

		// envia um alerta de que uma nova conexao foi estabelecida
		sendTelegramMessage("🟢 Nova conexão estabelecida! " + time.Now().Format("2006-01-02 15:04:05"))

		logEntry := fmt.Sprintf("\n\n===== [NOVA CONEXÃO] %s =====\n", time.Now().Format("2006-01-02 15:04:05"))
		logFile.WriteString(logEntry)

		serverConn, err := net.Dial("tcp", "localhost:9001")
		if err != nil {
			sendTelegramMessage("🔴 Falha na conexão com o servidor! " + err.Error())
			clientConn.Close()
			continue
		}

		handleConnection(clientConn, serverConn, logFile)
	}
}

func handleConnection(clientConn net.Conn, serverConn net.Conn, logFile *os.File) {
	defer clientConn.Close()
	defer serverConn.Close()

	clientReader := bufio.NewReader(clientConn)
	serverReader := bufio.NewReader(serverConn)
	var sessionMessages []string

	for {
		// le a mensagem do cliente
		msg, err := clientReader.ReadString('\n')
		if err != nil {
			fmt.Println("Erro ao ler do cliente:", err)
			break
		}

		// add a mensagem ao log e ao resumo
		logEntry := fmt.Sprintf("[%s] CLIENTE -> SERVIDOR: %s", time.Now().Format("2006-01-02 15:04:05"), msg)
		logFile.WriteString(logEntry)
		sessionMessages = append(sessionMessages, "→ "+strings.TrimSpace(msg)) // Remove "\n" log de depuração
		fmt.Println("Mensagem do cliente:", msg)

		// encaminha a mensagem para o servidor
		_, err = serverConn.Write([]byte(msg))
		if err != nil {
			fmt.Println("Erro ao enviar para o servidor:", err)
			break
		}

		// le a resposta do servidor
		response, err := serverReader.ReadString('\n')
		if err != nil {
			fmt.Println("Erro ao ler do servidor:", err)
			break
		}

		// adiciona a resposta ao log e ao resumo
		logEntry = fmt.Sprintf("[%s] SERVIDOR -> CLIENTE: %s", time.Now().Format("2006-01-02 15:04:05"), response)
		logFile.WriteString(logEntry)
		sessionMessages = append(sessionMessages, "← "+strings.TrimSpace(response))
		fmt.Println("Resposta do servidor:", response)

		// encaminha a resposta ao cliente
		_, err = clientConn.Write([]byte(response))
		if err != nil {
			fmt.Println("Erro ao enviar para o cliente:", err)
			break
		}
	}

	// notifica o telegram ao encerrar e envia o resumo (mensagens trocadas) durante a conexao
	if len(sessionMessages) > 0 {
		resumo := fmt.Sprintf(
			"<b>🔴 Conexão encerrada!</b>\n" +
				"<i>📝 Mensagens trocadas:</i>\n" +
				"<pre>" + strings.Join(sessionMessages, "\n") + "</pre>")

		sendTelegramMessage(resumo)
	} else {
		sendTelegramMessage("🔴 Conexão encerrada (sem mensagens trocadas).")
	}
}
