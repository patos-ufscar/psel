package balancer

import (
	"fmt"
	"loadbal/proxy"
	"net"
)

type Balancer struct {
	Host        string
	Servers     []string
	Connections []int // connections[n] is the amount of connections of servers[n]
	Strategy    Strategy
}

func New(host string, servers []string, strategy Strategy) *Balancer {
	return &Balancer{
		Host:        host,
		Servers:     servers,
		Connections: make([]int, len(servers)),
		Strategy:    strategy,
	}
}

func (b *Balancer) PickServer() int {
	server := b.Strategy(b)
	fmt.Printf("Picked server %q\n", b.Servers[server])

	return server
}

func (b *Balancer) Start() error {
	listener, err := net.Listen("tcp", b.Host)
	defer listener.Close()

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

		serverIndex := b.PickServer()
		serverHostname := b.Servers[serverIndex]

		if err != nil {
			fmt.Println("Failed to connect to new server:", err)
			clientConn.Close()
			continue
		}

		onClose := func() {
			b.Connections[serverIndex]--
		}

		proxy := proxy.New(clientConn, serverHostname, &onClose)
		go proxy.Start()

		b.Connections[serverIndex]++
	}
}
