package main

import (
	"fmt"
	"loadbal/server"
	"loadbal/balancer"
)

func main() {
	servers := []*server.Server{
		server.New("localhost:8001"),
	}

	balancer, err := balancer.New("localhost:8000", servers)

	if err != nil {
		panic(err)
	}

	balancer.Start()
	fmt.Println("Started")
}
