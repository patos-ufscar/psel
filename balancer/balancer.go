package balancer

import (
	"fmt"
)

type Balancer struct {
	Servers []string
	Connections []int  // connections[n] is the amount of connections of servers[n]
}

func New(servers []string) *Balancer {
	return &Balancer{
		Servers: servers,
		Connections: make([]int, len(servers)),
	}
}

func (b *Balancer) Pick(s Strategy) string {
	server := s(b)
	fmt.Printf("Picked server [%q]\n", server)

	return server
}
