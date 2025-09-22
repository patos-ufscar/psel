package common

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io"
	"net"
	"os"
)

func ProxyHandler(connClient net.Conn) {
	defer connClient.Close()

	// Carrega o par de chaves e o certificado da CA
	cert, err := tls.LoadX509KeyPair("../certs/proxy.crt", "../certs/proxy.key")

	if err != nil {
		fmt.Println("TLS error (load key pair):", err)
		return
	}

	ca, err := os.ReadFile("../certs/ca.crt")

	if err != nil {
		fmt.Println("TLS error (load authorities certificate ):", err)
		return
	}

	// Cria um conjunto de CAs confiáveis para validar o certificado do servidor
	pool := x509.NewCertPool()
	pool.AppendCertsFromPEM(ca)

	// Configura a conexão TLS
	tlsConf := &tls.Config{
		Certificates: []tls.Certificate{cert}, // Certificação que ela vai apresentar
		RootCAs:      pool,                    // CA que ela vai confiar
	}

	// Cria uma conn TLS com o server
	connServerTls, err := tls.Dial(NETWORK, ADDR_SERVER, tlsConf)

	if err != nil {
		fmt.Println("Error to create tls.dial conn:", err)
		ErrorHttpHandler(connClient, NOT_FOUND_STATUS)
		return
	}

	defer connServerTls.Close()

	// Goroutine para enviar os request do client para o server
	go io.Copy(connServerTls, connClient)

	// Manda a resposta do sever para o client
	io.Copy(connClient, connServerTls)

}
