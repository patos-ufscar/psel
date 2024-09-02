package com.ufscar.psor;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

public class Main {

    private static final int PORT = 1107;

    public static void main(String[] args) {
        try {
            try (ServerSocket serverSocket = new ServerSocket(PORT)) {
                System.out.println("escutando na porta: " + PORT);
                
                while (true) {
                    try (Socket clientSocket = serverSocket.accept();) {
                        System.out.println("cliente conectado: " + clientSocket.getInetAddress().getHostAddress());
                        handleClient(clientSocket);
                    }
                        
                }

            }
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    public static void handleClient(Socket clientSocket) throws IOException {
        try (OutputStream outputStream = clientSocket.getOutputStream(); 
            InputStream inputStream = clientSocket.getInputStream()) 
        {
            //outputStream.write("\nQuack, Quack, converse com o pato!\n  __\n<(o )___\n (  ._> /\n  `---' \n".getBytes());
            //byte[] buffer = new byte[2048];
            //inputStream.read(buffer);

            outputStream.write("HTTP/1.1 200 OK\r\n\r\n<h1>Quack, Quack! Voce achou o pato!<h1> <pre>  __\n<(o )___\n (  ._> /\n  `---' </pre>".getBytes()); 
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }
}