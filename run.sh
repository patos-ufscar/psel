#!/usr/bin/env bash

for port in 8081 8082 8083 8084 8085; do
	go run ./server -port "$port" &
done

go run ./balancer
