package proxy

import (
	"fmt"
	"io"
	"net"
)

type Proxy struct {
	clientConn net.Conn
	serverConn *net.Conn
	serverHost string
	Open       bool
	onClose    *func()
}

func New(clientConn net.Conn, serverHost string, onClose *func()) *Proxy {
	return &Proxy{
		Open:       false,
		clientConn: clientConn,
		serverHost: serverHost,
		onClose:    onClose,
	}
}

func (p *Proxy) Start() error {
	serverConn, err := net.Dial("tcp", p.serverHost)
	p.serverConn = &serverConn

	if err != nil {
		fmt.Println("Failed to connect to server:", err)
		p.clientConn.Close()
		return err
	}

	p.Open = true

	// io.Copy can (and will) be used here, but first I want
	// to implement a simple version by hand
	go p.copyStream(p.clientConn, serverConn)
	go p.copyStream(serverConn, p.clientConn)

	return nil
}

func (p *Proxy) copyStream(in, out net.Conn) {
	buf := make([]byte, 1024)

	for p.Open {
		n, err := in.Read(buf)

		if err == io.EOF {
			p.Close()
			break
		}

		if err != nil {
			// Connection most likely closed
			break
		}

		// TODO: properly handle this too
		out.Write(buf[:n])
	}
}

func (p *Proxy) Close() {
	p.clientConn.Close()
	(*p.serverConn).Close()
	p.Open = false

	if p.onClose != nil {
		(*p.onClose)()
	}
}
