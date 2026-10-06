package main

import (
	"bufio"
	"flag"
	"io"
	"load-balancer/internal/httpcore"
	"log"
	"net"
	"os"
	"strconv"
	"strings"
	"sync/atomic"
	"time"
)

type Backend struct {
	addr    string
	isAlive atomic.Bool
}

var Backends []*Backend

var counter uint64

func HealthCheck(endereco *Backend) {
	connection, err := net.DialTimeout("tcp", endereco.addr, 1*time.Second)
	if err != nil {
		endereco.isAlive.Store(false)
		log.Println("Error with Backend")
		return
	}
	endereco.isAlive.Store(true)
	connection.Close()

}

func runHealthcheck() {

	for {
		for _, backend := range Backends {
			go HealthCheck(backend)
		}
		time.Sleep(5 * time.Second)

	}
}

func nextBackend() *Backend {

	for i := 0; i < len(Backends); i++ {
		idx := (atomic.AddUint64(&counter, 1) - 1) % uint64(len(Backends))
		if Backends[idx].isAlive.Load() == true {
			return Backends[idx]
		}

	}
	return nil

}

func forwardbody(reader *bufio.Reader, connectionBackend net.Conn, contentLength string) error {
	numbytes, err := strconv.Atoi(contentLength)
	if err != nil {
		log.Println("Error while converting")
		return err
	}
	buffer := make([]byte, numbytes)
	_, err = io.ReadFull(reader, buffer)
	if err != nil {
		log.Println("error while take bytes")
		return err
	}
	_, err = connectionBackend.Write(buffer)
	if err != nil {
		log.Println("error sending resposne response of body")
		return err
	}
	return nil
}

func forwardToBackend(Request httpcore.Request, connection net.Conn, reader *bufio.Reader) {

	startedTime := time.Now()

	chosedBackend := nextBackend()
	if chosedBackend == nil {
		log.Println("All Backends is down")
		err := httpcore.SendResponse(connection, 503, "Service Unavaliable", "text/html", "503 Service Unavaliable")
		if err != nil {
			log.Println("Error whit Backends")
			return
		}
		return
	}

	connectionBackend, err := net.Dial("tcp", chosedBackend.addr)
	if err != nil {
		log.Println("error sending connection to backend")
		err = httpcore.SendResponse(connection, 500, "Internal Server Error", "text/html", "500 Internal Server Error")
		if err != nil {
			log.Println("Error of server")
			return
		}
		return
	}

	defer connectionBackend.Close()

	message := Request.Method + " " + Request.Uri + " " + Request.Version + "\r\n"

	for chave, valor := range Request.Header {
		message += chave + ": " + valor + "\r\n"

	}

	message += "\r\n"

	messageb := []byte(message)

	_, err = connectionBackend.Write(messageb)
	if err != nil {
		log.Println("Error sending answer")
		return
	}

	if Request.Method == "POST" {
		err = forwardbody(reader, connectionBackend, Request.Header["Content-Length"])
		if err != nil {
			log.Println("Error sending response")
			err = httpcore.SendResponse(connection, 400, "Bad Request", "text/html", "400 Bad Request")
			if err != nil {
				log.Println("Error sending response")
				return
			}
		}

	}

	readerBackend := bufio.NewReader(connectionBackend)

	statusline, err := readerBackend.ReadString('\n')
	if err != nil {
		log.Println("error while reading request line")
		return
	}

	messager := statusline
	fields := strings.Split(statusline, " ")
	statuscode := fields[1]
	statuscodeint, err := strconv.Atoi(statuscode)
	if err != nil {
		log.Println("error converting")
		return
	}
	messageb = []byte(messager)
	_, err = connection.Write(messageb)
	if err != nil {
		log.Println("error sending answer")
		return
	}

	_, err = io.Copy(connection, readerBackend)
	if err != nil {
		log.Println("error sending response")
		return
	}

	passedTime := time.Since(startedTime)

	log.Println(chosedBackend.addr, statuscodeint, passedTime)

}

const (
	HOST = "localhost"
	TYPE = "tcp"
)

func main() {
	portFlag := flag.String("port", "8080", "Porta do Loadbalancer")
	backendFlag := flag.String("backends", "localhost:8081,localhost:8082,localhost:8083", "Endereço dos Backends")
	flag.Parse()
	flagBackendsplited := strings.Split(*backendFlag, ",")

	for _, addr := range flagBackendsplited {
		Backends = append(Backends, &Backend{addr: addr})
	}
	for i := 0; i < len(flagBackendsplited); i++ {
		Backends[i].isAlive.Store(true)
	}

	listen, err := net.Listen(TYPE, HOST+":"+*portFlag)
	if err != nil {
		log.Println("Error", err)
		os.Exit(1)
	}
	defer listen.Close()

	go runHealthcheck()

	for {
		connection, err := listen.Accept()
		if err != nil {
			log.Println("Error stablishing connection", err)
			continue
		}
		go httpcore.Accepting_con(connection, forwardToBackend)
	}

}
