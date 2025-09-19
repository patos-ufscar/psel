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

	pool := x509.NewCertPool()
	pool.AppendCertsFromPEM(ca)

	tlsConf := &tls.Config{
		Certificates: []tls.Certificate{cert},
		RootCAs:      pool,
	}

	buffer := make([]byte, MAX_FILE_SIZE)
	n, err := connClient.Read(buffer)

	if err != nil {
		fmt.Println("Error to read buffer:", err)
		ErrorHttpHandler(connClient, NOT_FOUND_STATUS)
		return
	}

	request := buffer[:n]

	connServerTls, err := tls.Dial(NETWORK, ADDR_SERVER, tlsConf)

	if err != nil {
		fmt.Println("Error to create tls.dial conn:", err)
		ErrorHttpHandler(connClient, NOT_FOUND_STATUS)
		return
	}

	defer connServerTls.Close()

	fmt.Println(string(request))

	_, erro := connServerTls.Write(request)

	if erro != nil {
		fmt.Println("Error to write request from proxy to server:", erro)
		return
	}

	go io.Copy(connClient, connServerTls)

	io.Copy(connServerTls, connClient)
}
