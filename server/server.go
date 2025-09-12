package main

import (
	"fmt"
	"net"
)

func main() {
	listener, err := net.Listen(NETWORK, ADDR)

	if err != nil {
		fmt.Println("Error to listen ->", err)
		return
	}

	defer listener.Close()

	fmt.Println("Server is linstening on port 8080!")

	for {
		conn, err := listener.Accept()

		if err != nil {
			fmt.Println("Error to accept connection ->", err)
			return
		}

		go ConnectionHandler(conn)

	}
}
