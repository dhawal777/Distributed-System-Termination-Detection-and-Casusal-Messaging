public class Message implements java.io.Serializable {
	private int timestamp;
	private String message;
	private int source;
	private MetaData data;
	
	public Message(int time, String msg, int id) {
		this.timestamp = time;
		message = msg;
		this.source = id;
		data = null;
	}
	
	public Message(String msg, int time, int source, MetaData data) {
		this.timestamp = time;
		message = msg;
		this.source = source;
		this.data = data;
	}
	
	public int getTimestamp() {
		return this.timestamp;
	}
	
	public String getMessage() {
		return this.message;
	}
	
	public int getSource() {
		return this.source;
	}
	
	public MetaData getMetaData() {
		return this.data;
	}
	
	public void setMetaData(MetaData d) {
		this.data = d;
	}
}