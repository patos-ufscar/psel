package common

import (
	"fmt"
	"io"
	"net"
	"strings"
)

func ProxyHandler(connClient net.Conn) {
	defer connClient.Close()

	buffer := make([]byte, 65535)
	total, err := connClient.Read(buffer)

	if err != nil {
		fmt.Println("Client-Proxy lost connection!")
		return
	}

	request := buffer[:total]
	str_request := string(request)
	var formatedRequest string

	if strings.Contains(str_request, "/download") {
		formatedRequest = strings.Replace(str_request, "/download", "/proxy/download", 1)
	} else if strings.Contains(str_request, "/upload") {
		formatedRequest = strings.Replace(str_request, "/upload", "/proxy/upload", 1)
	} else {
		formatedRequest = strings.Replace(str_request, "/", "/proxy", 1)
	}

	connServer, err := net.Dial(NETWORK, ADDR_SERVER)

	if err != nil {
		ErrorHttpHandler(connClient, NOT_FOUND_STATUS)
	}

	_, erro := connServer.Write([]byte(formatedRequest))

	if erro != nil {
		fmt.Println("Error to write request from proxy to server:", erro)
	}
	defer connServer.Close()

	io.Copy(connClient, connServer)

}
