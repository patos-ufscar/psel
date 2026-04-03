package main

import (
	"loadbal/balancer"
)

func main() {
	servers := []string{"localhost:8001", "httpforever.com:80"}
	bal := balancer.New("localhost:8000", servers, balancer.Strategies.LeastConnections)

	err := bal.Start()

	if err != nil {
		panic(err)
	}
}
