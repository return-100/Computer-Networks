import java.net.Socket;
import java.io.*;

public class Client extends Thread
{
    File file;
    Socket clientsocket;
    FileInputStream fis;
    BufferedInputStream bis;
    InputStream in;
    OutputStream out;
    String message;
    String path;

    public Client(Socket clientsocket) {
        this.clientsocket = clientsocket;
        message = "";
        path = "/Users/return_100/IdeaProjects/HttpOffline/src";
    }

    public String getContent(String filename) {
        String str = "";
        String[] parts = filename.split("[/.]");

        if (filename.length() <= 1) {
            file = new File(path + "/index.html");

            if (file.length() == 0) {
                file = new File(path + "/error.html");
            }
        }
        else {
            file = new File(path + filename);

            if (file.length() == 0) {
                file = new File(path + "/error.html");
            }
            else if (parts.length > 2) {
                if (parts[2].equalsIgnoreCase("html")) {
                    str += "text/html";
                }
                else if (parts[2].equalsIgnoreCase("pdf")) {
                    str += "application/pdf";
                }
                else if (parts[2].equalsIgnoreCase("jpg")) {
                    str += "image/jpeg";
                }
                else if (parts[2].equalsIgnoreCase("tif")) {
                    str += "image/tiff";
                }
                else {
                    str += "image/" + parts[2].toLowerCase();
                }
            }
        }

        return str;
    }

    public void run() {
        try {
            in = clientsocket.getInputStream();
            out = clientsocket.getOutputStream();

            while (true) {
                message += (char) in.read();

                if (in.available() == 0)
                    break;
            }

            System.out.println(message);

            String[] parts = message.split(" ");

            if (parts[0].equals("GET")) {
                String str = getContent(parts[1]);
                fis = new FileInputStream(file);
                bis = new BufferedInputStream(fis);

                out.write(("HTTP/1.0 200 OK\r\n").getBytes());
                out.write(("Content-Type: " + str + "\r\n\r\n").getBytes());

                byte[] arr = new byte[(int) (file.length() + 1)];
                bis.read(arr, 0, (int) (file.length()));
                out.write(arr);
            }
            else {
                String postfile = "";
                String[] httpparts;
                String[] username = message.split("user=");
                String str = getContent("/index.html");
                fis = new FileInputStream(file);
                bis = new BufferedInputStream(fis);

                byte[] arr = new byte[(int) (file.length() + 1)];
                bis.read(arr, 0, (int) (file.length()));

                for (int i = 0; i < arr.length; ++i) {
                    postfile += (char)arr[i];
                }

                httpparts = postfile.split("Post->");
                postfile = httpparts[0] + "Post->" + username[1] + httpparts[1];

                byte[] brr = postfile.getBytes();

                out.write(("HTTP/1.0 200 OK\r\n").getBytes());
                out.write(("Content-Type: " + str + "\r\n\r\n").getBytes());
                out.write(brr);
            }

            out.flush();
            out.close();
            bis.close();
            in.close();
        }
        catch (IOException e) {

        }
    }
}
