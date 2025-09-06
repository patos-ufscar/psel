package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"strings"
)

const (
	NETWORK = "tcp"
	ADDR    = "localhost:8080"
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
		go handleConn(conn)
	}
}

func handleConn(conn net.Conn) {
	defer conn.Close()

	// slice of bytes to store the data from connection stream
	data := make([]byte, 4096)

	// read bytes from connection. n is the length of bytes read
	n, err := conn.Read(data)
	if err != nil || n == 0 {
		fmt.Println("Client close connection!")
		return
	}

	lines := strings.Split(string(data), "\r\n")
	request := strings.Split(lines[0], " ")
	method := request[0]
	filepath := request[1]
	if filepath == "/" {
		filepath = "./index.html"
	}

	if method == "GET" {
		file, err := os.Open(filepath)
		if err != nil {
			fmt.Fprintf(conn, "HTTP/1.1 404 Not Found\r\n\r\n")
			return
		}

		// formating the header
		stat, _ := file.Stat()
		fmt.Fprintf(conn, "HTTP/1.1 200 OK\r\n")
		fmt.Fprintf(conn, "Content-Type: text/html\r\n")
		fmt.Fprintf(conn, "Content-Length: %d\r\n", stat.Size())
		fmt.Fprintf(conn, "\r\n")
		// send the file content to connection
		io.Copy(conn, file)
	}
}
