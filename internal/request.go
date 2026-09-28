// a simple HTTP 1.1 parser built from scratch to be used by both the LB and servers
// contained inside the 'internal' package

package internal

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"strconv"
	"strings"
	"time"
)

// max size that a photo can occupy (jpeg files range from 300KB to 10MB, and PNG files from 1.5MB to 50+MB)
const MaxBodySize = 20 << 20 // 20 MiB (a modest size)

// max accepted time before dropping the connection
const TransferTimeout = 30 * time.Second

// max possible size of the header, to prevent exploits
const maxHeaderCount = 64

// lets the caller answer 413 instead of dropping the connection.
var ErrBodyTooLarge = errors.New("request body larger than the allowed maximum")

// struct of the HTTP request, the main components
type Request struct {
	Method  string
	Path    string
	Headers map[string]string // names lowercased, they are case-insensitive on the wire
	Body    []byte            // nil when the request had no body, raw bytes
}

// reads a header value by name, ignoring case
func (r *Request) Header(name string) string {
	return r.Headers[strings.ToLower(name)]
}

// reads the whole request to guarantee it's properly done, depends on other functions below
func ReadRequest(reader *bufio.Reader, maxBody int) (*Request, error) {
	method, path, err := readRequestLine(reader)
	if err != nil {
		return nil, err
	}

	headers, err := readHeaders(reader)
	if err != nil {
		return nil, err
	}

	body, err := readBody(reader, headers, maxBody)
	if err != nil {
		return nil, err
	}

	return &Request{Method: method, Path: path, Headers: headers, Body: body}, nil // fills in the struct, no errors
}

// actually writes and sends the request to the backend following HTTP 1.1 convention
func WriteRequest(w io.Writer, method, path, host string, body []byte, extraHeaders ...string) error {
	header := fmt.Sprintf("%s %s HTTP/1.1\r\nHost: %s\r\n", method, path, host)

	for _, extra := range extraHeaders {
		header += extra + "\r\n"
	}
	if len(body) > 0 {
		header += fmt.Sprintf("Content-Length: %d\r\n", len(body))
	}
	header += "Connection: close\r\n\r\n"

	if _, err := w.Write([]byte(header)); err != nil {
		return err
	}
	if len(body) == 0 {
		return nil
	}

	_, err := w.Write(body)
	return err
}

// reads and analyses the request Method and Path (ReadRequest)
func readRequestLine(reader *bufio.Reader) (method, path string, err error) {
	line, err := readLine(reader)
	if err != nil {
		return "", "", err
	}

	sections := strings.Fields(line)
	if len(sections) < 2 {
		return "", "", fmt.Errorf("malformed request line: %q", line)
	}

	return sections[0], sections[1], nil // method, path, no errors
}

// scans the block after the request line, stopping at the empty line (ReadRequest)
func readHeaders(reader *bufio.Reader) (map[string]string, error) {
	headers := make(map[string]string) // makes a string-string map (dictionary)

	for {
		line, err := readLine(reader) // actually reads the line (function at the end of the file)
		if err != nil {
			return nil, err
		}
		if line == "" {
			return headers, nil
		}
		if len(headers) >= maxHeaderCount { // too many Headers, likely exploit
			return nil, fmt.Errorf("more than %d headers", maxHeaderCount)
		}

		rawName, rawValue, found := strings.Cut(line, ":")
		if !found { // line lacks proper 'grammar' of HTTP header
			return nil, fmt.Errorf("malformed header line: %q", line)
		}
		name, value := strings.ToLower(strings.TrimSpace(rawName)), strings.TrimSpace(rawValue)

		// content-length is incongruent, therefore abort
		if previous, seen := headers[name]; seen && name == "content-length" && previous != value {
			return nil, fmt.Errorf("conflicting Content-Length headers")
		}

		headers[name] = value // maps to the map created earlier
	}
}

// reads the body and filters, reads only the number of bytes informed by the header
// no header, no reading (ReadRequest)
func readBody(reader *bufio.Reader, headers map[string]string, maxBody int) ([]byte, error) {
	// ignoring a chunked body would leave this parser disagreeing with the next one
	if encoding := headers["transfer-encoding"]; encoding != "" {
		return nil, fmt.Errorf("unsupported Transfer-Encoding: %q", encoding)
	}

	raw, present := headers["content-length"]
	if !present { // no body
		return nil, nil
	}

	length, err := strconv.Atoi(raw)
	if err != nil || length < 0 { // content can't be less then 0, and abort if there's an error
		return nil, fmt.Errorf("invalid Content-Length: %q", raw)
	}
	if length == 0 {
		return nil, nil
	}
	// before allocation, if body is to large, abort
	if length > maxBody {
		return nil, ErrBodyTooLarge
	}

	body := make([]byte, length) // allocate the body
	if _, err := io.ReadFull(reader, body); err != nil {
		return nil, fmt.Errorf("body shorter than its Content-Length: %w", err)
	}

	return body, nil
}

// actually reads the Header line and scans any incongruencies (ReadRequest)
func readLine(reader *bufio.Reader) (string, error) {
	line, err := reader.ReadSlice('\n')
	if errors.Is(err, bufio.ErrBufferFull) {
		return "", fmt.Errorf("header line longer than %d bytes", reader.Size())
	}
	if err != nil {
		return "", err
	}

	// ReadSlice returns a view into the buffer, so string() copies it out
	return strings.TrimRight(string(line), "\r\n"), nil
}
