package main

import (
	"fmt"
	"net"

	"github.com/oeduardopereira/psel/common"
)

func main() {
	listener, err := net.Listen(common.NETWORK, common.ADDR_SERVER)

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
