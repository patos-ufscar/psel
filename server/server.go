package main

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"os"

	"github.com/oeduardopereira/psel/common"
)

func main() {

	cert, err := tls.LoadX509KeyPair("../certs/server.crt", "../certs/server.key")

	if err != nil {
		fmt.Println("TLS error (load key pair):", err)
	}

	ca, err := os.ReadFile("../certs/ca.crt")

	if err != nil {
		fmt.Println("TLS error (load authorities certificate ):", err)
	}

	pool := x509.NewCertPool()
	pool.AppendCertsFromPEM(ca)

	tlsConf := &tls.Config{
		Certificates: []tls.Certificate{cert},
		RootCAs:      pool,
		ClientCAs:    pool,
		ClientAuth:   tls.RequireAndVerifyClientCert,
	}

	listener, err := tls.Listen(common.NETWORK, common.ADDR_SERVER, tlsConf)

	if err != nil {
		fmt.Println("Error to listen ->", err)
		return
	}

	defer listener.Close()

	fmt.Println("Server is linstening on port", common.ADDR_SERVER)

	for {
		conn, err := listener.Accept()

		if err != nil {
			fmt.Println("Error to accept connection ->", err)
			return
		}

		go common.ConnectionHandler(conn)

	}
}
