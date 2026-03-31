package server

import (
	"net"
	"io"
)

type Server struct {
	connections int
	host string
}

func New(host string) *Server {
	return &Server{
		host: host,
		connections: 0,
	}
}

func (s *Server) Request(content []byte) (response []byte, err error) {
	conn, err := net.Dial("tcp", s.host)

	if err != nil {
		return
	}

	defer conn.Close()

	_, err = conn.Write(content)

	if err != nil {
		// TODO: handle this error - try again
		return
	}

	s.connections++
	defer func() { s.connections-- }()

	for {
		buf := make([]byte, 4096)
		n, err := conn.Read(buf)

		if n == 0 || err == io.EOF {
			break
		}

		if err != nil {
			return response, err
		}

		response = append(response, buf[:n]...)
	}

	return
}
