package main

import (
	"fmt"
	"net"

	"github.com/oeduardopereira/psel/common"
)

func main() {

	listener, err := net.Listen(common.NETWORK, common.ADDR_PROXY)

	if err != nil {
		fmt.Println("Error to proxy liste:", err)
		return
	}

	defer listener.Close()

	fmt.Println("Proxy is lintening on:", common.ADDR_PROXY)

	for {

		conn, err := listener.Accept()

		if err != nil {
			fmt.Println("Error to accept proxy connection:", err)
		}

		go common.ProxyHandler(conn)
	}
}
