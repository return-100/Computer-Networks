import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {
    public static void main(String[] args) throws IOException {

        ServerSocket serverSocketsocket = new ServerSocket(8080);

        while (true) {
            Socket clientsocket = serverSocketsocket.accept();
            new Client(clientsocket).start();
        }
    }
}