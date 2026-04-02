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

func (s *Server) Send(content []byte) (response []byte, err error) {
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


	return
}
