package common

const (
	NETWORK     = "tcp"
	ADDR_SERVER = "localhost:9001"
	ADDR_PROXY  = "localhost:8080"
)

const (
	OK_STATUS                 = "200 OK"
	NOT_FOUND_STATUS          = "404 Not Found"
	METHOD_NOT_ALLOWED_STATUS = "405 Method Not Allowed"
	PAYLOAD_TOO_LARGE         = "413 Payload Too Large"
	UNSUPPORTED_MEDIA_TYPE    = "415 Unsupported Media Type"
	INTERNAL_SERVER_ERROR     = "500 Internal Server Error"
)

const (
	MAX_FILE_SIZE = 1024 * 1024
)

const (
	UPLOAD_DIRECTORY   = "../files/uploads"
	DOWNLOAD_DIRECTORY = "../files/downloads"
	HOMEPAGE           = "../frontend/index.html"
)
