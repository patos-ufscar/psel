package common

import (
	"fmt"
	"net"
)

func ConnectionHandler(conn net.Conn) {
	defer conn.Close()

	method, action, filename, err := RequestInfos(conn)

	if err != nil {
		fmt.Println("Request erro:", err)
		return
	}

	switch method {
	case "GET":
		getHandler(conn, action, filename)

	default:
		ErrorHttpHandler(conn, METHOD_NOT_ALLOWED_STATUS)
	}

}

// Lida com a action passada no request 
// Caso seja /download, ele retorna o arquivo pedido para download
func getHandler(conn net.Conn, action string, filename string) {
	switch action {

	case "/download":
		filepath := DOWNLOAD_DIRECTORY + "/" + filename
		RenderPage(conn, filepath, true, filename)

	case "/":
		RenderPage(conn, HOMEPAGE, false, "")

	default:
		ErrorHttpHandler(conn, NOT_FOUND_STATUS)
	}
}
