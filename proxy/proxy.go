package proxy

import (
	"net"
	"io"
	"fmt"
)

type Proxy struct {
	clientConn net.Conn
	serverConn net.Conn
	open bool
}

// I don't really enjoy constructors that yeld and may throw errors,
// but it's the way I found to avoid checking whether serverConn
// exists. I'll likely rewrite this in the future.
func New(clientConn net.Conn, serverHost string) (*Proxy, error) {
	serverConn, err := net.Dial("tcp", serverHost)

	if err != nil {
		fmt.Println("Failed to connect to server:", err)
		clientConn.Close()
		return nil, err
	}

	proxy := &Proxy{
		clientConn: clientConn,
		serverConn: serverConn,
		open: true,
	}

	// io.Copy can (and will) be used here, but first I want
	// to implement a simple version by hand
	go proxy.copyStream(clientConn, serverConn)
	go proxy.copyStream(serverConn, clientConn)

	return proxy, nil
}

func (p *Proxy) copyStream(in, out net.Conn) {
	buf := make([]byte, 1024)

	for {
		n, err := in.Read(buf)

		if err == io.EOF {
			p.Close()
			break
		}

		if err != nil {
			// TODO: properly handle this
			fmt.Println("Failed to read client data:", err)
			break
		}

		// TODO: properly handle this too
		out.Write(buf[:n])
	}
}

func (p *Proxy) Close() {
	p.clientConn.Close()
	p.serverConn.Close()
	p.open = false
}
