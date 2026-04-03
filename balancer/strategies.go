package balancer

import (
	"math/rand"
)

type Strategy func(balancer *Balancer) int

func static(b *Balancer) int {
	return 0
}

func random(b *Balancer) int {
	return rand.Intn(len(b.Servers))
}

func leastConnections(b *Balancer) int {
	// Hopefully O(n) won't be a problem
	leastIndex := 0
	least := b.Connections[leastIndex]

	for i, connections := range b.Connections {
		if connections < least {
			least = connections
			leastIndex = i
		}
	}

	return leastIndex
}

var Strategies = map[string]Strategy {
	"Static": static,
	"Random": random,
	"LeastConnections": leastConnections,
}
