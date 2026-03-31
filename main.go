package main

import "fmt"
import "loadbal/server"

func main() {
	s := server.New("localhost:8080")
	response, err := s.Request([]byte("content delivery"))

	if err != nil {
		panic(err)
	}

	fmt.Printf("%q", response)
}
