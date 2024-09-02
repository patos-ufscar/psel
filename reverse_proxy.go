/*
package main

import (
	"crypto/tls"
	"fmt"
	"net/http"
	"net/url"
)

func init() {
	http.DefaultTransport.(*http.Transport).TLSClientConfig = &tls.Config{InsecureSkipVerify: true}
}

func main() {
	demoURL, err := url.Parse("http://172.17.0.2")
	if err != nil {
		panic(err)
	}

	proxy := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		r.Host = demoURL.Host
		r.URL.Host = demoURL.Host
		r.URL.Scheme = demoURL.Scheme
		r.RequestURI = ""
		_, erro := http.DefaultClient.Do(r)
		if erro != nil {
			w.WriteHeader(http.StatusInternalServerError)
			fmt.Println(w, erro)
			return
		}
	})
	http.ListenAndServe(":8080", proxy)
}
*/