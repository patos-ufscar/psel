package proxy

import (
	"net"
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
		return nil, err
	}

	return &Proxy{
		clientConn: clientConn,
		serverConn: serverConn,
		open: false,
	}, nil
}
