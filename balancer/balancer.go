package balancer

import (
	"fmt"
	"net"
	"loadbal/proxy"
)

type Balancer struct {
	Host string
	Servers []string
	Connections []int  // connections[n] is the amount of connections of servers[n]
	Strategy Strategy
}

func New(host string, servers []string, strategy Strategy) *Balancer {
	return &Balancer{
		Host: host,
		Servers: servers,
		Connections: make([]int, len(servers)),
		Strategy: strategy,
	}
}

func (b *Balancer) Pick(s Strategy) string {
	server := s(b)
	fmt.Printf("Picked server [%q]\n", server)

	return server
}

func (b *Balancer) Start() error {
	listener, err := net.Listen("tcp", b.Host)

	if err != nil {
		return err
	}

	for {
		clientConn, err := listener.Accept()

		if err != nil {
			fmt.Println("Failed to connect to new client:", err)
			continue
		}

		fmt.Println("New client connected:", clientConn.RemoteAddr().String())

		bestServerHost := b.Pick(b.Strategy)

		if err != nil {
			fmt.Println("Failed to connect to new server:", err)
			clientConn.Close()
			continue
		}

		go proxy.New(clientConn, bestServerHost)
	}
}
