package common

const (
	NETWORK     = "tcp"
	ADDR_SERVER = "localhost:9001" // porta do server
	ADDR_PROXY  = "localhost:8080" // porta da proxy
)

const (
	OK_STATUS                 = "200 OK"
	NOT_FOUND_STATUS          = "404 Not Found"
	METHOD_NOT_ALLOWED_STATUS = "405 Method Not Allowed"
	INTERNAL_SERVER_ERROR     = "500 Internal Server Error"
)

const (
	DOWNLOAD_DIRECTORY = "../files/downloads"
	HOMEPAGE           = "../frontend/index.html"
)
