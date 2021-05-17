import java.util.Scanner;
import java.net.Socket;
import java.io.IOException;

public class DateClient {
	public static void main(String[] args) throws IOException {
		try {
			if (args.length != 1) {
				System.err.println("Pass the server IP as the sole command line argument");
				return;
			}
			Socket socket = new Socket(args[0], 5001);
			Scanner in = new Scanner(socket.getInputStream());
			System.out.println("Server response: " + in.nextLine());
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
