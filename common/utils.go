package common

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"os"
	"path"
	"strings"
)

func RequestInfos(conn net.Conn) (string, string, string, error) {
	reader := bufio.NewReader(conn)
	requestLines, err := reader.ReadString('\n')

	if err != nil {
		return "", "", "", err
	}

	requestLines = strings.TrimSpace(requestLines)
	lines := strings.Split(requestLines, " ")

	if len(lines) < 2 {
		return "", "", "", fmt.Errorf("error: bad request format -> %s", requestLines)
	}

	method := lines[0]
	requestLine := strings.Split(lines[1], "?")

	if len(requestLine) < 1 {
		return "", "", "", fmt.Errorf("error: couldnt read action")
	}

	action := requestLine[0]

	if method != "GET" && method != "POST" {
		ErrorHttpHandler(conn, METHOD_NOT_ALLOWED_STATUS)
	}

	if action != "/download" && action != "/" && action != "/upload" {
		ErrorHttpHandler(conn, NOT_FOUND_STATUS)
	}

	filename := ""

	if len(requestLine) > 1 {
		filename = strings.Split(requestLine[1], "=")[1]
	}

	return method, action, filename, nil
}

func formatHeader(filepath string, file *os.File, status string, todownload bool, filename string, size int) string {
	filetype := GetFileType(filepath)
	stat, _ := file.Stat()
	var res string

	if todownload {
		res = fmt.Sprintf("HTTP/1.1 %s\r\n"+
			"Content-Type: application/octet-stream\r\n"+
			"Content-Disposition: attachment; filename=\"%s\"\r\n"+
			"Content-Length: %d\r\n\r\n", status, filename, stat.Size())

	} else {
		_, lengthHtml := renderFilesToDownload()
		totalLength := stat.Size() + int64(lengthHtml) + int64(size)
		res = fmt.Sprintf("HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", status, filetype, totalLength)

	}

	return res
}

func GetFileType(file string) string {
	ext := path.Ext(file)
	var filetype string

	switch ext {
	case ".html":
		filetype = "text/html"

	case ".png":
		filetype = "image/png"

	case ".txt":
		filetype = "text/plain"

	default:
		filetype = "application/octet-stream"
	}

	return filetype
}

func RenderPage(conn net.Conn, filepath string, todownload bool, filename string) {
	file, err := os.Open(filepath)

	if err != nil {
		ErrorHttpHandler(conn, NOT_FOUND_STATUS)
	}

	defer file.Close()
	var header string

	if todownload {
		header = formatHeader(filepath, file, OK_STATUS, todownload, filename, 0)
		fmt.Fprint(conn, header)
		io.Copy(conn, file)

	} else {
		dataFile, err := os.ReadFile(filepath)

		if err != nil {
			ErrorHttpHandler(conn, NOT_FOUND_STATUS)
		}

		strFile := string(dataFile)
		html, _ := renderFilesToDownload()
		body := strings.Replace(strFile, "<!-- FILES_PLACEHOLDER -->", html, 1)

		header = formatHeader(filepath, file, OK_STATUS, todownload, filename, len(body))
		fmt.Fprint(conn, header)
		fmt.Fprint(conn, body)
	}
}

func getFilesToDownload() ([]string, error) {
	files, err := os.ReadDir(DOWNLOAD_DIRECTORY)

	if err != nil {
		return nil, err
	}

	var allFiles []string

	for _, file := range files {

		if !file.IsDir() {
			allFiles = append(allFiles, file.Name())
		}
	}
	return allFiles, nil
}

func renderFilesToDownload() (string, int) {
	files, _ := getFilesToDownload()
	html := ""

	if len(files) < 1 {
		html = "<html><body><h3> Não há arquivos para download :( </h3></body></html>"
		return html, len(html)

	} else {
		html = "<html><body><ul>"

		for _, file := range files {
			html += "<li><a href=\"/download?file=" + file + "\">" + " + " + file + "</a></li>"
		}

		html += "</ul></body</html>"
	}

	return html, len(html)
}

func ErrorHttpHandler(conn net.Conn, status string) {
	var body string

	switch status {
	case NOT_FOUND_STATUS:
		body = "<html><body><h3> Erro 404 Not Found </h3>"

	case METHOD_NOT_ALLOWED_STATUS:
		body = "<html><body><h3> Erro 405 Method Not Allowed</h3>"

	case INTERNAL_SERVER_ERROR:
		body = "<html><body><h3> Erro 500 Internal Server Error</h3>"

	default:
		body = "<html><body><h3> Pô vei, deu algum b.o. ai 🤷</h3><"
	}

	body += "<br><br><a href=\"/\">Voltar para pagina inicial</a></body></html>"

	res := fmt.Sprintf("HTTP/1.1 %s\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", status, len(body), body)
	fmt.Fprint(conn, res)
}
