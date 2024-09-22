package com.ufscar.psor;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

public class Main {

    private static final int PORT = 8080;
    private static final String ROOT = "reverse-proxy-file-server/public"; // diretorio base para os arquivos
    private static final String FILE = "test.html"; // nome do arquivo

    public static void main(String[] args) {
        try {
            try (ServerSocket serverSocket = new ServerSocket(PORT)) {
                System.out.println("escutando na porta: " + PORT);
                
                // loop para aceitar conexões
                while (true) {
                    try (Socket clientSocket = serverSocket.accept();) {
                        System.out.println("cliente conectado: " + clientSocket.getInetAddress().getHostAddress());
                        handleRequest(clientSocket);
                    }

                }

            }
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    public static void handleRequest(Socket clientSocket) throws IOException {
        try (OutputStream outputStream = clientSocket.getOutputStream(); InputStream inputStream = clientSocket.getInputStream()) {

            byte[] buffer = new byte[2048];
            int bytesRead = inputStream.read(buffer);
            if (bytesRead <= 0) { // evitar requisições vazias
                System.out.println("requisição vazia");
                return;
            }
            String request = new String(buffer, 0, bytesRead);

            //debug
            //System.out.println("client request: \n" + request);

            String[] requestLines = request.split("\r\n"); // divide a requisição em linhas

            String[] requestLine = requestLines[0].split(" "); // divide a primeira linha da requisição em três partes (método, caminho, versão)
            String requestedFile = requestLine[1]; // como estamos lidando apenas com GET, vamos direto ao caminho

            // solicitação vazia = test.html
            if (requestedFile.equals("/")) {
                requestedFile = "/" + FILE;
            }

            // caminho do arquivo solicitado parseado
            File file = new File(ROOT + requestedFile);
            System.out.println("arquivo solicitado: " + file.getAbsolutePath());

            if (file.exists() && !file.isDirectory()) {

                outputStream.write("HTTP/1.1 200 OK\r\n".getBytes());
                outputStream.write("Content-Type: text/html\r\n\r\n".getBytes());

                // uso de um FileInputStream para evitar fazer a conversão do arquivo para String depois para Bytes de novo
                try (FileInputStream fileInputStream = new FileInputStream(file)) {

                    byte[] fileBufferByte = new byte[1024];
                    int length;
                    while ((length = fileInputStream.read(fileBufferByte)) > 0) {
                        outputStream.write(fileBufferByte, 0, length);
                    }
                }
            } else {
                String notFound = "HTTP/1.1 404 Not Found\r\n\r\n<h1>404 File Not Found</h1>";
                outputStream.write(notFound.getBytes());
            }

            outputStream.flush();
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }
}
