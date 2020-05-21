Compile Program
g++ arbitrary.cpp -pthread

Run the Program
./a.out <own PID>

Example : 
./a.out 1
./a.out 2
./a.out 3


It Support Arbitrary Node Failure. 
And There is No loss of MSG
So If msg sent, But receiving node Failed, So when Receiving Node wakesup it will get this msg. 


Config.txt 
It contain list of process with its pid and IP, port on which it is running.
<PID> <IP of Process> <PORT>
1 127.0.0.1 8088
2 127.0.0.1 8089
