package main

import (
	"fmt"
	"loadbal/proxy"
	"net"
)

func main() {
	listener, err := net.Listen("tcp", "localhost:8000")

	if err != nil {
		panic(err)
	}

	for {
		clientConn, err := listener.Accept()

		if err != nil {
			fmt.Println("Failed to connect to new client:", err)
			continue
		}

		fmt.Println("New client connected:", clientConn.RemoteAddr().String())

		bestServerHost := "localhost:8001" // hardcoded for now

		if err != nil {
			fmt.Println("Failed to connect to new server:", err)
			clientConn.Close()
			continue
		}

		go proxy.New(clientConn, bestServerHost)
	}
}
