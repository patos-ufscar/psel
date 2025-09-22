#!/bin/bash
# Cria, se não existir, o diretório certs/
CERT_DIR="./certs"
mkdir -p "$CERT_DIR"

# Cria a autoridade 
echo "Creating certificate authority..."
openssl req -new -x509 -sha256 -days 365 -nodes -keyout "$CERT_DIR/ca.key" -out "$CERT_DIR/ca.crt" -subj "/CN=My-Psel-CA"

# Cria a chave e certificado do server, e salva no diretorio
echo "Creating server key and server csr..."
openssl genrsa -out "$CERT_DIR/server.key" 2048
openssl req -new -sha256 -key "$CERT_DIR/server.key" -out "$CERT_DIR/server.csr" -subj "/CN=localhost" -addext "subjectAltName = DNS:localhost,IP:127.0.0.1"

# assina o certificado do server pela CA
echo "Signing server certificate with CA..."
openssl x509 -req -in "$CERT_DIR/server.csr" -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" -CAcreateserial -out "$CERT_DIR/server.crt" -days 365 -sha256 -extfile <(echo "subjectAltName=DNS:localhost,IP:127.0.0.1")

# Cria a chave e certificado da proxy, e salva no diretorio
echo "Creating proxy key and csr..."
openssl genrsa -out "$CERT_DIR/proxy.key" 2048
openssl req -new -sha256 -key "$CERT_DIR/proxy.key" -out "$CERT_DIR/proxy.csr" -subj "/CN=localhost" -addext "subjectAltName = DNS:localhost,IP:127.0.0.1"

# assina o certificado da proxy pela CA
echo "Signing proxy certificate with CA..."
openssl x509 -req -in "$CERT_DIR/proxy.csr" -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" -CAcreateserial -out "$CERT_DIR/proxy.crt" -days 365 -sha256 -extfile <(echo "subjectAltName=DNS:localhost,IP:127.0.0.1")

echo "Certificados gerados com sucesso na pasta '$CERT_DIR'!"