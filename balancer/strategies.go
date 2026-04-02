package balancer

import (
	"math/rand"
)

type Strategy func(balancer *Balancer) string

func static(b *Balancer) string {
	return b.Servers[0]
}

func random(b *Balancer) string {
	return b.Servers[rand.Intn(len(b.Servers))]
}

var Strategies = struct {
	Static Strategy
	Random Strategy
} {
	Static: static,
	Random: random,
}
