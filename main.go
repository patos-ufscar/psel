package main

import (
	"loadbal/balancer"
	"encoding/json"
	"os"
	"fmt"
)

type Config struct {
	Port int `json:port`
	Strategy string `json:strategy`
	Servers []string `json:servers`
}

func main() {
	fileContent, err := os.ReadFile("./config.json")
	if err != nil {
		panic(err)
	}

	var config Config
	err = json.Unmarshal(fileContent, &config)
	if err != nil {
		panic(err)
	}

	strategy, strategyExists := balancer.Strategies[config.Strategy]
	if !strategyExists {
		panic(fmt.Sprintf("Invalid strategy %q in config.json", config.Strategy))
	}

	bal := balancer.New(fmt.Sprintf(":%d", config.Port), config.Servers, strategy)

	err = bal.Start()
	if err != nil {
		panic(err)
	}
}
