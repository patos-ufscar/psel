package com.ufscar.psor;

import java.io.IOException;
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
                    try (Socket clientSocket = serverSocket.accept(); OutputStream outputStream = clientSocket.getOutputStream()) {
                        outputStream.write("\nQuack, Quack!\n  __\n<(o )___\n (  ._> /\n  `---' ".getBytes());
                        outputStream.flush();
                    }
                }
            }
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }
}