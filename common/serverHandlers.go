package common

import (
	"fmt"
	"net"
)

func ConnectionHandler(conn net.Conn) {
	defer conn.Close()

	method, action, filename, err := RequestInfos(conn)

	if err != nil {
		fmt.Println("Proxy-Server lost connection!")
		return
	}

	switch method {
	case "GET":
		getHandler(conn, action, filename)

	case "POST":
		/* a ideia era fazer um POST de arquivos,
		   porem não estava muito afim de ficar
		   manipulando eles. O default do é GET mesmo*/
	default:
		getHandler(conn, action, filename)
	}

}

func getHandler(conn net.Conn, action string, filename string) {
	switch action {
	case "/upload":
		//RenderPage(conn, HOMEPAGE, false, "")

	case "/download":
		filepath := DOWNLOAD_DIRECTORY + "/" + filename
		RenderPage(conn, filepath, true, filename)

	default:
		RenderPage(conn, HOMEPAGE, false, "")
	}
}
