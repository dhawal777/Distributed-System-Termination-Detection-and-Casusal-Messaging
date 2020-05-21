import java.net.*;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.*;

public class main1 {
	private static int minDelay;
	private static int maxDelay;
	private static HashMap<Integer, MetaData> list = new HashMap<Integer, MetaData>();
	private static ArrayList<Integer> v_timestamps = new ArrayList<Integer>();
	private static HashMap<Integer, ArrayList<CausalMessage>> holdBackQueue = new HashMap<Integer, ArrayList<CausalMessage>>();
	private static final Object queueLock = new Object();
	
	public static String getTime() {
		return new SimpleDateFormat("HH:mm:ss").format(Calendar.getInstance().getTime());
	}
	public static void addProcessToList(String input, int id) {
		String[] info = input.split(" ");
		MetaData data = new MetaData(info, null, null, false);
		list.put(id, data);
		holdBackQueue.put(id, new ArrayList<CausalMessage>());
		v_timestamps.add(id-1, 0);
	}
	public static void scanConfigFile(int id) {
		File file = new File("../config_file.txt");
		try {
			Scanner scanner = new Scanner(file);
			String[] line = scanner.nextLine().split(" ");
			String s = line[0].substring(line[0].indexOf("(") + 1);
		minDelay = Integer.parseInt(s.substring(0, s.indexOf(")")));
		s = line[1].substring(line[1].indexOf("(") + 1);
		maxDelay = Integer.parseInt(s.substring(0, s.indexOf(")")));
			String input = "";
			boolean found = false;
			int num = 1;
			while (scanner.hasNext()) {
				if (num == id) {
					input = scanner.nextLine();
					addProcessToList(input, id);
					found = true;
				}
				else {
					input = scanner.nextLine();
					addProcessToList(input, Integer.parseInt(input.substring(0, 1)));
				}
				num++;
			}
			scanner.close();
			
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}
	public static void startClient(final int id, final String serverName, final int port) {
        (new Thread() {
            @Override
            public void run() {
            	readAndSendMessages(id);
            }
        }).start();
	}
	
	public static void readAndSendMessages(int id) {
    	try {
    		BufferedReader stdIn = new BufferedReader(new InputStreamReader(System.in));
        	String input;
			while ((input = stdIn.readLine()) != null) {
				final String message = input;
				final int clientId = id;
		        (new Thread() {
		            @Override
		            public void run() {
						if (checkMulticastInput(message)) {
							String msg = message.substring(6);
							multicast(msg, clientId);
						}
						else if (!message.isEmpty()) {
							System.err.println("msend <message>");
						}
		            }
		        }).start();
			}
			stdIn.close();
		} catch (NumberFormatException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	private static ArrayList<Integer> incrementTimestamp(int id) {
		synchronized (queueLock) {
			int time = v_timestamps.get(id-1)+1;
			v_timestamps.set(id-1, time);
		return v_timestamps;
		}
	}
	
	private static void printVectorTimes(ArrayList<Integer> arr) {
		synchronized (queueLock) {
			for (int i = 0; i < arr.size(); i++) {
				System.out.print(arr.get(i));
			}
			System.out.println("");
		}
	}
	public static void sendMessage(CausalMessage m) {
		try {
			String[] destinationInfo = m.getMetaData().getProcessInfo();
			int destination = Integer.parseInt(destinationInfo[0]);
			MetaData data = list.get(destination);
			
			if (!data.isOpen()) {
				Socket s = new Socket(destinationInfo[1], Integer.parseInt(destinationInfo[2]));
				data.setSocket(s);
				data.setWriter(new ObjectOutputStream(data.getSocket().getOutputStream()));
				data.setOpen(true);
			}
			
			data.getWriter().reset();
			
			data.getWriter().writeObject(m);
			data.getWriter().flush();
			System.out.println(m.getTimestamp());
			System.out.println("Message " + m.getMessage() + " is sent to process " + destination  + "from "+m.getSource()+", system time is " + getTime());
		} catch (IOException e) {
			System.err.println("ERROR:");
			e.printStackTrace();
		}
	}
	public static void multicast(String message, int source) {
		ArrayList<Integer> times = incrementTimestamp(source);
		for (int i = 0; i < list.size(); i++) {
			MetaData data = list.get(i+1);
			if (data != null && source != (i+1)) {
				CausalMessage m = new CausalMessage(message, times, source, data);
				sendMessage(m);
			}
		}
	}
	public static boolean checkMulticastInput(String input) {
		if (input.length() >= 6 && input.substring(0, 5).equals("msend")) {
			input = input.substring(5);
			return Character.isWhitespace(input.charAt(0));
		}
		else 
			return false;
	}
	public static void startServer(String serverName, final int port) {
        (new Thread() {
            @Override
            public void run() {
                ServerSocket ss;
                try {
                    ss = new ServerSocket(port);
                    while (true) {
	                    final Socket s = ss.accept();
	                    (new Thread() {
	                    	@Override
	                    	public void run() {
	                    		receiveMessages(s);
	                    	}
	                    }).start();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }).start();
	}
	public static void receiveMessages(final Socket s) {
		try {
			ObjectInputStream in = new ObjectInputStream(s.getInputStream());
	        CausalMessage msg;
			while (true && (msg = (CausalMessage)in.readObject()) != null) {
				final CausalMessage m = msg;
                (new Thread() {
                	@Override
                	public void run() {
                		unicastReceive(m, s);
                	}
                }).start();
			}
		} catch (IOException e) {
			e.printStackTrace();
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		}
	}
	public static void unicastReceive(CausalMessage m, Socket s) {
		MetaData data = m.getMetaData();
		int source = m.getSource();
		int id = Integer.parseInt(data.getProcessInfo()[0]);
		
		if (list.get(source).getSocket() == null)
			list.get(source).setSocket(s);
		
		if (delayMessage(m, source)) {
			deliverMessage(m, source, s);
		}	
	}
	public static boolean delayMessage(CausalMessage m, int source) {
		
		return checkTimeStamps(m, source);
	}
	public static String serverip = "127.0.0.1";
    public static String currentUser = "";

    public static Socket connect(String ip, int port) {
        Socket sock = null;
        int ctr = 0;
        boolean conn = false;
        while(ctr<5 && !conn){
            System.out.println("Connecting "+"ip:"+port+" "+(ctr+1));
        try {
            sock = new Socket(ip, port);
            conn = true;
        } catch (IOException u) {
            System.out.println(u);
            ctr+=1;
        }
        }
        return sock;
    }

    public static int uploadTCP(String filename) throws IOException {
        Socket fsock = connect(serverip, 5000);
        BufferedOutputStream outToClient = new BufferedOutputStream(fsock.getOutputStream());
        if (outToClient != null) {
            File myFile = new File( filename );
            byte[] mybytearray = new byte[(int) myFile.length()];

            FileInputStream fis = null;

            try {
                fis = new FileInputStream(myFile);
            } catch (FileNotFoundException ex) {
                // Do exception handling
            }
            BufferedInputStream bis = new BufferedInputStream(fis);

            try {
                bis.read(mybytearray, 0, mybytearray.length);
                outToClient.write(mybytearray, 0, mybytearray.length);
                outToClient.flush();
                outToClient.close();
            } catch (IOException ex) {
                // Do exception handling
            }
            bis.close();
            fis.close();
        }
        fsock.close();
        return 1;
    }

    public static int uploadUDP(String filename) throws IOException {
        byte sdata[] = new byte[1024];
        DatagramSocket dsoc = new DatagramSocket();
        InetAddress ip = InetAddress.getByName(serverip);
        FileInputStream inputStream = new FileInputStream(filename);
        int nRead = 0;
        while ((nRead = inputStream.read(sdata)) != -1) {
//            dosc.send(new DatagramPacket(nRead, 1, ip, 9000));
            dsoc.send(new DatagramPacket(sdata, sdata.length, ip, 9000));
            System.out.println("sent" + nRead);
            if (nRead == 0) {
                sdata = "END".getBytes();
                dsoc.send(new DatagramPacket(sdata, sdata.length, ip, 9000));
                System.out.println("Sent!");
            }
        }
        inputStream.close();
        dsoc.close();
        return 0;
    }
	public static boolean checkTimeStamps(CausalMessage m, int source) {
		ArrayList<Integer> mesgTimes = m.getTimestamp();
		
		synchronized (queueLock) {
			int v_time = v_timestamps.get(source - 1);
			int mesgTime = mesgTimes.get(source - 1);
			
			printVectorTimes(v_timestamps);
			printVectorTimes(mesgTimes);
			int greater = 0;
			int less = 0;
			if (mesgTime == (v_time + 1)) {
				for (int i = 0; i < mesgTimes.size(); i++) {
					if (i != source - 1) { 
						if (v_timestamps.get(i) < mesgTimes.get(i))
							greater++;
						else if (v_timestamps.get(i) > mesgTimes.get(i))
							less++;
					}
				}
				if (greater >= 1 && less >= 1) {
					return true;
				}
				else if (greater == 0) {
					return true;
				}
			}
			
			holdBackQueue.get(source).add(m);
			return false;
		}
	}
	
	public static void deliverMessage(CausalMessage m, int source, Socket s) {
		String message = m.getMessage();

		v_timestamps.set(source - 1, m.getTimestamp().get(source-1));
		
		System.out.println("Message  \"" + message + "\"is received from process " + source + ", system time is " + getTime());

		checkHoldbackQueue(source);
	}
	public static void checkHoldbackQueue(int id) {
		CausalMessage msg = null;
		boolean deliver = false;
		synchronized (queueLock) {
			for (int i = 0; i < holdBackQueue.size(); i++) {
				ArrayList<CausalMessage> msgs = holdBackQueue.get(i+1);
				if (msgs != null && msgs.size() > 0) {
					for (int j = 0; j < msgs.size(); j++) {
						msg = msgs.get(j);
						int v_time = v_timestamps.get(msg.getSource() - 1);
						if (msg.getTimestamp().get(i) == (v_time + 1) && checkTimeStamps(msg, msg.getSource())) {
							deliver = true;
							msgs.remove(j);
							break;
						}
					}
				}
			}
		}
		
		if (deliver) {
			deliverMessage(msg, msg.getSource(), msg.getMetaData().getSocket());
		}
		
	}
	public static void main(String[] args) throws IOException {
		int id = Integer.parseInt(args[0]);
		scanConfigFile(id);
		String[] info = list.get(id).getProcessInfo();
		for ( String elem : info ) 
		{
  				System.out.println("Node details : "+elem);
		}
		startServer(info[1], Integer.parseInt(info[2]));
		startClient(id, info[1], Integer.parseInt(info[2]));
	}
}
