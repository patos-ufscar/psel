package httpcore

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"strconv"
	"strings"
)

type Request struct {
	Method  string
	Uri     string
	Header  map[string]string
	Version string
}

func ParseRequest(reader *bufio.Reader) (Request, error) {
	var req Request
	req.Header = make(map[string]string)

	requestline, err := reader.ReadString('\n')
	if err != nil {
		log.Println("error while reading request line")
		return req, err
	}

	requestline = strings.Trim(requestline, "\r\n")
	var fields []string
	fields = strings.Split(requestline, " ")

	if len(fields) != 3 {
		log.Println("error spliting string")

		return req, fmt.Errorf("Malformated Requisition")
	}

	req.Method = fields[0]
	req.Uri = fields[1]
	req.Version = fields[2]

	var line string

	for {
		line, err = reader.ReadString('\n')
		if err != nil {
			log.Println("error while read headers", err)
			return req, err
		}

		line = strings.Trim(line, "\r\n")

		if line == "" {
			break
		}

		var partes []string
		partes = strings.SplitN(line, ":", 2)

		if len(partes) != 2 {
			log.Println("Erros spliting parts")
			return req, fmt.Errorf("Malformated Requisition")

		}

		partes[1] = strings.Trim(partes[1], " ")

		req.Header[partes[0] /*chave*/] = partes[1] /*valor*/

	}
	return req, nil
}

func SendResponse(connection net.Conn, statuscode int, statusmessage string, contentType string, body string) error {

	bodyb := len(body)
	bodys := strconv.Itoa(bodyb)
	statuscodeString := strconv.Itoa(statuscode)
	message := "HTTP/1.1" + " " + statuscodeString + " " + statusmessage + "\r\n" + "Content-type: " + contentType + "\r\n" + "Content-Length: " + bodys + "\r\n" + "\r\n" + body
	bodybyte := []byte(message)
	_, err := connection.Write(bodybyte)
	if err != nil {
		log.Println("error sending answer")
		return err
	}

	return nil

}

func Accepting_con(connection net.Conn, reading_methods func(Request, net.Conn, *bufio.Reader)) {
	defer connection.Close()

	reader := bufio.NewReader(connection) //mesma coisa que o buffer fazia

	Request, err := ParseRequest(reader)
	if err != nil {
		log.Println("error calling function")
		err = SendResponse(connection, 400, "Bad Request", "text/html", "400 Bad Request")
		if err != nil {
			log.Println("400 Bad Request")
			return
		}
		return
	}

	reading_methods(Request, connection, reader)
}
