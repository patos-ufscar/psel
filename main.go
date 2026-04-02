package main

import (
	"fmt"
	"net"
	"loadbal/proxy"
	"loadbal/balancer"
)

func main() {
	bal := balancer.New([]string{"localhost:8001"})
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

		bestServerHost := bal.Pick(balancer.Strategies.Static)

		if err != nil {
			fmt.Println("Failed to connect to new server:", err)
			clientConn.Close()
			continue
		}

		go proxy.New(clientConn, bestServerHost)
	}
}
