#include<bits/stdc++.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <dirent.h>


using namespace std;

class Process
{
 public:

  int id;
  string state;      //"IDLE"  OR  "ACTIVE"
  double weight;     //initiallly 0 for NORMAL, 1 for CONTROL 
  string type;       // "CONTROL" OR "NORMAL"
 
  // Default Contructor
  Process()
  {

  };

  // Parameterised contructor
  Process(int id1,string state1,double weight1,string type1)
  {
    
    id=id1;
    state=state1;
    weight=weight1;
    type=type1;
    
  }

};

class socketclass
{
  public:
    char *ip; //IP Address
    int port; //Port Address

    socketclass()
    {
        // ip="";
        port = 0;
    }

    void setsocketdata(string sc)
    {
        vector<string> tokens;

        stringstream check1(sc);

        string intermediate;

        // Tokenizing w.r.t. space ' '
        while (getline(check1, intermediate, ':'))
        {
            tokens.push_back(intermediate);
        }

        string strip = tokens[0];
        ip = new char[strip.length() + 1];
        strcpy(ip, strip.c_str());
        port = stoi(tokens[1]);
    }
};

// Process Object
Process p;

//Hash table to store :: ProcessID --> < IP, PORT >
map<int,pair<string,int>> processDetails;


ofstream myfile;
//Printing Log into a file
void writeLog(string log)
{
    myfile << log <<endl;   
}

// Checking Double value equal with epsilon error in floating points
bool double_equals(double a, double b, double epsilon = 0.001)
{
    return std::abs(a - b) < epsilon;
}

// When Receiving Weight 
void *weightCalculation(void *socket_desc)
{

    int new_socket = *(int *)socket_desc;
    char buffer[1024] = {0};
    read(new_socket, buffer, 1024);
    string sweight = string(buffer);

    double int_weight = stod(sweight);
    p.state = "ACTIVE";
    cout<<"\n\nTotal Weight before Receive: "<<p.weight<<endl;
    cout<<"Received Weight : "<< int_weight <<endl;
    p.weight += int_weight;
    cout<<"Total Weight After Received: "<<p.weight<<endl<<endl;
    
    double a=1;
    if(p.type=="CONTROL" && double_equals(p.weight,a))
    {
      cout<<"******** Termination Done ***********"<<endl;
      writeLog("******** Termination Done ***********");
    }
  
 
}

// Function Runs in background to Accept connection from any other Process / Control Agent.
void *background_con_wait_service(void *socket_desc)
{
    string cli_socket = *(string *)socket_desc;
    socketclass cserversocket;
    pthread_t thread_id;
    cserversocket.setsocketdata(cli_socket);
    // cout<<"Listing on IP:Port = "<<cli_socket<<endl;

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed in seeder");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(cserversocket.ip);
    address.sin_port = htons(cserversocket.port);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed in seeder");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // accept connection from any process
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Error in accept connection in seeder");
            exit(EXIT_FAILURE);
        }

        // writelog("******Connection accepted in seeder *******");

        // create new thread to serve request and do the weight calculation accordingly
        if (pthread_create(&thread_id, NULL, weightCalculation, (void *)&new_socket) < 0)
        {
            perror("\ncould not create thread in seeder\n");
        }
        if (new_socket < 0)
        {
            perror("accept failed in seeder");
        }
    }

    return socket_desc;
}

//Read from config file to take each process's IP and PORT
void readfromFile()
{
  std::ifstream file("config.txt");
    std::string str;    
    while (std::getline(file, str)) 
    {
          std::istringstream iss(str);
          std::string sub;
          
          int count=0;
          string id;
          string ip;
          string port;
        
          while(iss >> sub)
          {
            if(count==0)
               id=sub;
            if(count==1)
              ip=sub;
            if(count==2)
              port=sub;
            
            count++;
          }
          
        
          int id1;
          stringstream geek(id); 
          int y = 0; 
          geek >> y; 
          
          stringstream geek1(port); 
          int x = 0; 
          geek1 >> x; 
          processDetails[y]=make_pair(ip,x);

    }

    // cout<< processDetails[1].first<<processDetails[1].second<<endl;
    // cout<< processDetails[2].first<<processDetails[2].second<<endl;
}

// Send Weight message to Other Process or Controlling Agent
int sendMsg(string socketAdd, double weight)
{
    socketclass sc;
    // cout<<"Weight sending to the Socket : "<<socketAdd<<endl;

    sc.setsocketdata(socketAdd);

    int sock = 0;
        struct sockaddr_in serv_addr;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error in client side\n");
            return -1;
        }

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(sc.port);

        //Convert IPv4 and IPv6 addresses from text to binary form
        // CHANGE IP
        if (inet_pton(AF_INET, sc.ip , &serv_addr.sin_addr) <= 0)
        {
            printf("\nClient File  : Invalid address/ Address not supported \n");
            return -1;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\nConnection Failed in client side\n");
            return -1;
        }
      
      string sweight = to_string(weight);
      char *send_weight = new char[sweight.length() + 1];
      strcpy(send_weight, sweight.c_str());

      send(sock , send_weight , strlen(send_weight) , 0 ); 

      printf("Weight sent !!! \n");
      return 1; 
}

int main(int argc, char const *argv[]) 
{ 

      if(argc < 4)
      {
          cout<<"Invalid Number of Arguments"<<endl;
          exit(0);
      }

      myfile.open("logs.txt", ios::app); 

      string input_id=argv[1]; // processID
      int process_id = stoi(input_id);
      string type=argv[2];  // Type Process "CONTROL" ,  "NORMAL"
      string controller_ID_S=argv[3]; // ControllerID
  
      readfromFile();
      //create new thread from client which act as seeder(server) to provide data to others

      string clientsocketstr = processDetails[process_id].first + ":" + to_string(processDetails[process_id].second);
      cout<<"------------------- My Details ---------------------"<<endl;
      cout<<"Process ID : "<<process_id<<endl;
      cout<<"Socket : "<<clientsocketstr<<endl;
      cout<<"Process Type : "<<type<<endl;
      cout<<"----------------------------------------------------"<<endl;

      pthread_t cserverid;
      if (pthread_create(&cserverid, NULL, background_con_wait_service, (void *)&clientsocketstr) < 0)
      {
          perror("\ncould not create thread in client side\n");
      }

      

      double init_weight=0;
      string init_state = "IDLE";
      if(type=="CONTROL")
      {
          init_weight=1;
          init_state = "ACTIVE";
          writeLog("Controlling Agent start with ProcessID : " + to_string(process_id));
      }
      else
      {
          writeLog("Normal Process start with ProcessID : " + to_string(process_id));
      }
      

      p.id = process_id;
      p.state = init_state;
      p.weight = init_weight;
      p.type = type;

        while(1)
        {
          
          int permission; 
          cout<<"\nDo you want to send weight to any other process(0=NO / 1=YES) : ";
            
            //Can be 0/1
            cin>>permission;
            if(permission==1)
            {
              if(p.state=="IDLE")
              {
                cout<<"You are not allowed to send untill you become active :( "<<endl;
              }
              else if(p.state=="ACTIVE")
              {
                int c;
                cout<<"Whom do u want to send"<<endl;
                cout<<"1.For controlling Agent"<<endl;
                cout<<"2.For Other process"<<endl;
                cin>>c;
                if(c==1)
                {
                  
                    int controllerId = stoi(controller_ID_S);
                    string controller_ip = processDetails[controllerId].first;
                    int controller_port = processDetails[controllerId].second;
                    
                    string socketAdd= controller_ip + ":" +to_string(controller_port);
                    cout<<"\nTotal Weight before send : "<<p.weight<<endl;
                    cout<<"Weight Sending : "<<p.weight<<endl;
                    sendMsg(socketAdd, p.weight);
                    writeLog("C(Weight="+ to_string(p.weight) +")" + " msg sent from process "+to_string(process_id)+ " to controlling_agent "+ controller_ID_S);
                    p.state="IDLE";
                    p.weight=0;
                    cout<<"Total Weight After send : "<<p.weight<<endl;
                }
                else
                {
                  
                  int pid;
                  cout<<"Please enter the processID to whom you want to send : ";
                  cin>>pid;
                  string receivingProcess_ip=processDetails[pid].first;
                  int receivingProcess_port=processDetails[pid].second;
                  

                  srand((unsigned) time(0));
                  double result = ((double)(1 + ((rand() % 6))))/7;
                  double weight_send=result*p.weight;
                  cout<<"\nTotal Weight before send : "<<p.weight<<endl;
                  cout<<"Weight Sending : "<<weight_send<<endl;
                  string socketAdd= receivingProcess_ip + ":" +to_string(receivingProcess_port);
                  sendMsg(socketAdd, weight_send);
                  writeLog("B(Weight="+ to_string(weight_send) +")" + " msg sent from process "+to_string(process_id)+ " to the process "+ to_string(pid));
                  p.weight-=weight_send;
                  cout<<"Total Weight After send : "<<p.weight<<endl;
            
                }
                
              }
              
            }
        }
      
      myfile.close();
} 

