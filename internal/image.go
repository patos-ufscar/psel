// Handles the way images are received by the application, properly filters them out
// and makes the new name so that they can be stored in the folder

package internal

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"path/filepath"
	"strings"
)

// verifies if the image extension is .png, .jpg/.jpeg
func HasImageExtension(name string) bool {
	switch strings.ToLower(filepath.Ext(name)) {
	case ".png", ".jpg", ".jpeg":
		return true
	default:
		return false
	}
}

// guarantees that the images don't contain invalid formats
func SafeImageName(raw string) (string, error) {
	name := filepath.Base(strings.TrimSpace(raw))

	// ponctuations that exploit Linux file system
	if name == "" || name == "." || name == ".." || strings.ContainsAny(name, `/\`) {
		return "", fmt.Errorf("unusable file name: %q", raw)
	}

	if !HasImageExtension(name) {
		return "", fmt.Errorf("only .png, .jpg and .jpeg are accepted, got %q", filepath.Ext(name))
	}

	// verifies if the file's name doesn't contain illegal characters
	for _, letter := range name {
		allowed := letter == '.' || letter == '_' || letter == '-' ||
			(letter >= '0' && letter <= '9') ||
			(letter >= 'a' && letter <= 'z') ||
			(letter >= 'A' && letter <= 'Z')
		if !allowed {
			return "", fmt.Errorf("file name contains a character that is not allowed: %q", letter)
		}
	}

	return name, nil
}

// verifies the actual raw bytes, otherwise the user could fake the files
func IsImage(body []byte) bool {
	png := []byte{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A}
	jpeg := []byte{0xFF, 0xD8, 0xFF}

	return bytes.HasPrefix(body, png) || bytes.HasPrefix(body, jpeg)
}

// small system to add a little SHA256 digest after the original name file, to prevent copies more accurately
func StoredName(name string, body []byte) string {
	sum := sha256.Sum256(body)
	digest := hex.EncodeToString(sum[:4])
	extension := filepath.Ext(name)

	return strings.TrimSuffix(name, extension) + "-" + digest + extension
}
