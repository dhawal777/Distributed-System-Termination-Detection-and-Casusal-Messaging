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
 
  // Default Contructor
  Process()
  {

  };

  // Parameterised contructor
  Process(int id1)
  {
    id=id1;
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


//Hash table to store :: ProcessID --> vector< MSG >
map< int,vector<string> > ds;


// ofstream myfile;
// //Printing Log into a file
// void writeLog(string log)
// {
//     myfile << log <<endl;   
// }


// When Receiving Weight 
void *weightCalculation(void *socket_desc)
{

    int new_socket = *(int *)socket_desc;
    char buffer[1024] = {0};
    read(new_socket, buffer, 1024);
    string rec_msg = string(buffer);
    cout<<"\n\nRecieved Msg : "<<rec_msg<<endl;

    string send_ack_str="ACK";
    char *send_ack = new char[send_ack_str.length() + 1];
    strcpy(send_ack, send_ack_str.c_str());
    send(new_socket , send_ack , strlen(send_ack) , 0 ); 
    close(new_socket);
 
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
int sendMsg(string socketAdd, string msg, int pid)
{
        socketclass sc;
        // cout<<"Called sendMsg()"<<endl;
        // cout<<"Msg sending to the Socket : "<<socketAdd<<endl;

        sc.setsocketdata(socketAdd);

        int sock = 0;
        struct sockaddr_in serv_addr;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            // printf("\n Socket creation error in client side\n");
            // close(sock);
            return -1;
        }

        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(sc.port);

        //Convert IPv4 and IPv6 addresses from text to binary form
        // CHANGE IP
        if (inet_pton(AF_INET, sc.ip , &serv_addr.sin_addr) <= 0)
        {
            // printf("\nClient File  : Invalid address/ Address not supported \n");
            return -1;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            // printf("\nConnection Failed in client side\n");
            // close(sock);
            return -1;
        }
      
      char *send_msg = new char[msg.length() + 1];
      strcpy(send_msg, msg.c_str());

      send(sock , send_msg , strlen(send_msg) , 0 ); 
      char buffer[1024];
      int valread = read( sock , buffer, 1024); 
    //   cout<<"ReadVal : "<<valread<<endl;
      if ( valread < 0) 
      {
          return -1;
      }
      string msg_rec = string(buffer);
      close(sock);
      return 1; 
}

void* checkQueue_Msg(void *socket_desc)
{
    // cout<<"checkQueue_Msg called"<<endl;
    while(1)
    {

        for(auto &it : ds)
        {
            vector<string> vt_msg = it.second;
            if(vt_msg.size()>0)
            {
                for(int i=vt_msg.size()-1;i>=0;i--)
                {
                    string msg = vt_msg[i];
                    int pid = it.first;
                    string receivingProcess_ip=processDetails[pid].first;
                    int receivingProcess_port=processDetails[pid].second;
                    string socketAdd= receivingProcess_ip + ":" +to_string(receivingProcess_port);
                    // cout<<socketAdd<<" : "<<msg<<endl;
                    int ack = sendMsg(socketAdd, msg, pid);
                    if(ack<0)
                    {
                        break;
                    }
                    else
                    {
                        // cout<<"tata"<<endl;
                        vt_msg.pop_back();
                    }   
                }
                ds[it.first] = vt_msg;
            }
        }
        usleep(5*1000000);
    }
}

int main(int argc, char const *argv[]) 
{ 

      if(argc < 2)
      {
          cout<<"Invalid Number of Arguments"<<endl;
          exit(0);
      }

    //   myfile.open("logs.txt", ios::app); 

      string input_id=argv[1]; // processID
      int process_id = stoi(input_id);
    
  
      readfromFile();

      string clientsocketstr = processDetails[process_id].first + ":" + to_string(processDetails[process_id].second);
      cout<<"------------------- My Details ---------------------"<<endl;
      cout<<"Process ID : "<<process_id<<endl;
      cout<<"Socket : "<<clientsocketstr<<endl;
      cout<<"----------------------------------------------------"<<endl;

      pthread_t cserverid;
      if (pthread_create(&cserverid, NULL, background_con_wait_service, (void *)&clientsocketstr) < 0)
      {
          perror("\ncould not create thread in client side\n");
      }

      pthread_t check_msg_thread;
      if (pthread_create(&check_msg_thread, NULL, checkQueue_Msg, (void *)&clientsocketstr) < 0)
      {
          perror("\ncould not create thread in client side\n");
      }


        while(1)
        {
          
          int pid; 
          cout<<"\nEnter the process ID to whom you want to send msg : ";
          cin>>pid;
          string receivingProcess_ip=processDetails[pid].first;
          int receivingProcess_port=processDetails[pid].second;
          string msg;
          cout<<"\nEnter the msg : ";
          cin>>msg;
          usleep(7 * 1000000); 
          string socketAdd= receivingProcess_ip + ":" +to_string(receivingProcess_port);
        //   cout<<"Socket : "<<socketAdd<<endl;
          int ack = sendMsg(socketAdd, msg, pid);
          if(ack<0)
          {
            cout<<"Adding into Queue"<<endl;
            ds[pid].push_back(msg);
          }
          else
            cout<<"Msg sent !!!"<<endl;

        }
            
    //   myfile.close();
} 

