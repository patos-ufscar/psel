package balancer

import (
	"loadbal/server"
	"net"
	"fmt"
	"io"
)

type balancer struct {
	host string
	servers []*(server.Server)
	listener net.Listener
}

func New(host string, servers []*server.Server) (*balancer, error) {
	listener, err := net.Listen("tcp", host)

	if err != nil {
		return nil, err
	}	

	return &balancer{
		host: host,
		servers: servers,
		listener: listener,
	}, nil
}

func (b *balancer) getBestServer() (*server.Server) {
	return b.servers[0]
}

func (b *balancer) handleConnection(conn net.Conn) {
	defer conn.Close()
	buf := []byte{}

	for {
		temp := make([]byte, 1024)
		n, err := conn.Read(temp)

		if err == io.EOF {
			break
		}

		buf = append(buf, temp[:n]...)
		if buf[-4:-1]
	}

	// routing the data to the server
	bestServer := b.getBestServer()

	for {
		buf := make([]byte, 4096)
		n, err := conn.Read(buf)

		if n == 0 || err == io.EOF {
			break
		}

		if err != nil {
			return response, err
		}

		response = append(response, buf[:n]...)
	}

	if err != nil {
		// TODO: handle this error properly
		fmt.Println("Failed to request server:", err)
		return
	}

	_, err = conn.Write(response)

	if err != nil {
		// TODO: handle this error properly
		fmt.Println("Failed to respond client:", err)
		return
	}
}

func (b *balancer) Start() {
	for {
		// stablishing connection with new client
		conn, err := b.listener.Accept()

		if err != nil {
			fmt.Println("Failed to listen to new connection:", err)
			continue
		}

		fmt.Println("Started new connection")
		go b.handleConnection(conn)
	}
}
