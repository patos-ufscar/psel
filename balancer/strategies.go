package balancer

type Strategy func(balancer *Balancer) string

func static (b *Balancer) string {
	return b.Servers[0]
}

var Strategies = struct {
	Static Strategy
} {
	Static: static,
}
