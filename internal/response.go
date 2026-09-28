// response parsing

package internal

import (
	"fmt"
	"io"
)

// writes the response and closes the connection
func WriteResponse(w io.Writer, status, contentType string, body []byte, extraHeaders ...string) error {
	header := fmt.Sprintf(
		"HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n",
		status, contentType, len(body),
	)

	for _, extra := range extraHeaders {
		header += extra + "\r\n"
	}
	header += "Connection: close\r\n\r\n"

	if _, err := w.Write([]byte(header)); err != nil {
		return err
	}

	_, err := w.Write(body)
	return err
}

// covers the common case: a short message with nothing attached (erros, for example)
func WriteText(w io.Writer, status, message string) error {
	return WriteResponse(w, status, "text/plain", []byte(message))
}
