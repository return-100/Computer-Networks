import java.io.*;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Base64;
import java.util.HashMap;
import java.util.Map;
import java.util.Scanner;

public class SMTP {

    static Scanner sc=new Scanner(System.in);
    static BufferedReader in;
    static PrintWriter pr;
    static String currentState;
    static BufferedReader sender;
    static BufferedReader receiver;
    static BufferedReader data;
    static BufferedReader fileData;
    static Map<Character,String> mp=new HashMap<>();
    static boolean attachment=false;

    public static void main(String[] args) throws UnknownHostException, IOException {
        sender=new BufferedReader(new FileReader("src/Sender.txt"));
        receiver =new BufferedReader(new FileReader("src/Receiver.txt"));
        data=new BufferedReader(new FileReader("src/Data.txt"));
        fileData=new BufferedReader(new FileReader("src/FileData.txt"));
        mp.put('1',"HELO");
        mp.put('2',"MAIL FROM:");
        mp.put('3',"RCPT TO:");
        mp.put('4',"DATA");
        mp.put('5',"QUIT");
        String mailServer = "smtp.sendgrid.net";
        InetAddress mailHost = InetAddress.getByName(mailServer);
        InetAddress localHost = InetAddress.getLocalHost();
//        Socket smtpSocket = new Socket(mailHost,587);
        Socket smtpSocket=new Socket();
        try
        {
            smtpSocket.connect(new InetSocketAddress(mailHost,587),20000);
        }catch (Exception e)
        {
            System.out.println("Connection Timeout");
            System.exit(0);
        }
        in =  new BufferedReader(new InputStreamReader(smtpSocket.getInputStream()));
        pr = new PrintWriter(smtpSocket.getOutputStream(),true);
        String initialID = in.readLine();
        System.out.println(initialID);
        //pr.flush();
        String str;
        pr.println("AUTH LOGIN");
        str=in.readLine();
        pr.println("YXBpa2V5");
        str=in.readLine();
        pr.println("U0cuOWdGNFN6eHZTS3U1b3FsS3FZb2c2Zy5yTFpwZzY2dGtDa2llMmhHSmVPTzdZUlZzV1EzU0hvemplQVQxUVR5aVVr");
        str=in.readLine();
        System.out.println(str);
        currentState="process";
        while (!currentState.equalsIgnoreCase("quit"))
        {
            if (currentState.equalsIgnoreCase("process"))
                process();
            else
                readData();
        }
    }

    public static void readData() throws IOException
    {
        String str="";
        if(attachment)
        {
            while (true)
            {
                str =fileData.readLine();
                if(str==null)
                    break;
                pr.println(str);
            }
            pr.println("");
            pr.println("");
            File imageFile = new File("src/buet.png");
            FileInputStream imageStream = new FileInputStream(imageFile);
            byte[] imageData = new byte[(int) imageFile.length()];
            imageStream.read(imageData);
            str = Base64.getEncoder().encodeToString(imageData);
            pr.println(str);
            pr.println("...");
            pr.println("--simple boundary--");
            pr.println(".");
        }
        else
        {
            while(true)
            {
                str=data.readLine();
                if(str==null)
                    break;
                pr.println(str);
            }
        }
        str=in.readLine();
        System.out.println(str);
        currentState="process";
    }

    public static void process() throws IOException
    {
        String str="";
        System.out.println("Selection:\n1.HELO\n2.MAIL FROM:\n3.RCPT TO:\n4.DATA\n5.QUIT\n6.Attachment:"+attachment);
        str=sc.nextLine();
        char ch=str.charAt(0);
        if(ch<'1'||ch>'6')
        {
            System.out.println("Please Enter a Valid Input");
            return;
        }
        if(ch=='6')
        {
            attachment=!attachment;
            return;
        }
        str=mp.get(ch);
        if(ch=='2')
            str+=sender.readLine();
        else if(ch=='3')
            str+=receiver.readLine();
        System.out.println("You:"+str);
        pr.println(str);
        if(ch=='4')
        {
            str=in.readLine();
            System.out.println("Server:"+str);
            if(str.charAt(0)=='3')
                currentState="data";
            return;
        }
        if(ch=='5')
            currentState="quit";
        str=in.readLine();
        System.out.println("Server:"+str);
    }
}
