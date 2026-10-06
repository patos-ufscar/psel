package main

import (
	"bufio"
	"io"
	"load-balancer/internal/httpcore"
	"log"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

func archivelist() (string, error) {
	dir, err := os.ReadDir("files")
	if err != nil {
		return "", err
	} else {
		var lista strings.Builder
		for _, arquivo := range dir {
			if arquivo.IsDir() == true {
				continue
			} else {
				nomearquivo := arquivo.Name()
				lista.WriteString("<li><a href=\"/" + nomearquivo + "\">" + nomearquivo + "</a></li>")

			}

		}
		return lista.String(), nil

	}
}

func reading_methods(Request httpcore.Request, connection net.Conn, reader *bufio.Reader) {
	if Request.Method == "GET" || Request.Method == "HEAD" {
		log.Println("Method:", Request.Method)
		log.Println("Uri:", Request.Uri)
		log.Println("Version: ", Request.Version)
		log.Println("Headers:", Request.Header)

		if Request.Uri == "/" {
			caminho := "files/index.html"
			caminhob, err := os.ReadFile(caminho)

			caminhoext := filepath.Ext(caminho)
			content, existe := mimemaps[caminhoext]
			if existe == false {
				content = "application/octet-stream"
			}

			if err != nil {
				log.Println("not found archive", err)

				err = httpcore.SendResponse(connection, 404, "not found", "text/html", "404 not found")
				if err != nil {
					log.Println("error requesting")
					return
				}
				return
			}

			lista, err := archivelist()
			if err != nil {
				err = httpcore.SendResponse(connection, 500, "Internal Server Error", "text/html", "500 Internal Server Error")
				if err != nil {
					log.Println("Error sendind response")
					return
				}
				return
			} else {
				caminhoString := string(caminhob)
				stringfinal := strings.Replace(caminhoString, "<!-- lista de arquivos sera inserida aqui-->", lista, 1)
				log.Println(caminhoString)
				err = httpcore.SendResponse(connection, 200, "OK", content, stringfinal)
				if err != nil {
					log.Println(stringfinal)
				}
			}
		} else {

			caminho := "files" + Request.Uri

			if strings.Contains(Request.Uri, "..") {
				err := httpcore.SendResponse(connection, 400, "Bad Request", "text/html", "400 Bad Request")
				if err != nil {
					log.Println("path transversal")
					return
				}
				return
			} else {
				caminhob, err := os.ReadFile(caminho)
				caminhoext := filepath.Ext(caminho)
				content, existe := mimemaps[caminhoext]
				if existe == false {
					content = "application/octet-stream"
				}

				if err != nil {
					log.Println("not found archive", err)

					err = httpcore.SendResponse(connection, 404, "not found", "text/html", "404 not found")
					if err != nil {
						log.Println("error requesting")
						return
					}
					return
				} else {
					caminhoString := string(caminhob)
					log.Println(caminhoString)
					err = httpcore.SendResponse(connection, 200, "OK", content, caminhoString)
					if err != nil {
						log.Println("error sending archive")
					}
				}
			}
		}

	} else if Request.Method == "POST" {
		numbytes, err := strconv.Atoi(Request.Header["Content-Length"])
		if err != nil {
			log.Println("Error while converting")
			//adicionar erro 400
			err = httpcore.SendResponse(connection, 400, "Bad Request", "text/html", "400 Bad Request")
			if err != nil {
				log.Println("Error in POST request")
			}
			return
		} else {
			buffer := make([]byte, numbytes)
			_, err := io.ReadFull(reader, buffer)
			if err != nil {

				log.Println("error while take bytes", err)
				err = httpcore.SendResponse(connection, 400, "Bad Request", "text/html", "400 Bad Request")
				if err != nil {
					log.Println("Bad Request")
				}
				return
			} else if numbytes != 0 {
				caminhoPost := "files" + Request.Uri
				if Request.Uri == "" || strings.Contains(Request.Uri, "..") == true {
					err = httpcore.SendResponse(connection, 400, "Bad Request", "text/html", "400 Bad Request")
					if err != nil {
						log.Println("error of Bad Request")
						return
					}
					return
				}
				err = os.WriteFile(caminhoPost, buffer, 0644)
				if err != nil {
					log.Println("error sending archive", err)
					err = httpcore.SendResponse(connection, 500, "Internal Server Error", "text/html", "500 Internal Server Error")
					if err != nil {
						log.Println("error of Service Unavaliable")
						return
					}
					return
				} else {
					err = httpcore.SendResponse(connection, 201, "Resource created", "text/html", "201 Resource Created")
					if err != nil {
						log.Println("error creating resource")
						return
					}
					return
				}
			} else if numbytes == 0 {
				err = httpcore.SendResponse(connection, 400, "Bad Request", "text/html", "400 Bad Request")
				if err != nil {
					log.Println("Content-Length = 0")
					return
				}
				return
			}

		}

	} else {
		err := httpcore.SendResponse(connection, 501, "Not Implemented", "text/html", "501 Not implemented")
		if err != nil {
			log.Println("Not implemented request")
		}
		return

	}
}

const (
	HOST = "localhost"
	TYPE = "tcp"
)

func main() {
	var PORT string

	if len(os.Args) < 2 {
		log.Println("Error")
		os.Exit(1)
	} else {
		PORT = os.Args[1]
	}

	listen, err := net.Listen(TYPE, HOST+":"+PORT)
	if err != nil {
		log.Print("Error", err)
		os.Exit(1)
	}
	defer listen.Close()

	for {
		connection, err := listen.Accept()
		if err != nil {
			log.Print("Error stablishing connection", err)
			continue
		}
		go httpcore.Accepting_con(connection, reading_methods)

	}

}
