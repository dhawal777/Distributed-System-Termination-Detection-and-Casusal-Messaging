NOTE : Please See video in laptop for better voice quality and picture quality (Preferable in VLC).

Compile Program
g++ termination_detection.cpp -pthread

Run the Program
./a.out <own PID> <TYPE of Process> <Controlling_Agent_PID>

Example : 
./a.out 1 CONTROL 1
./a.out 2 NORMAL 1
./a.out 3 NORMAL 1



Config.txt 
It contain list of process with its pid and IP, port on which it is running.
<PID> <IP of Process> <PORT>
1 127.0.0.1 8088
2 127.0.0.1 8089


Log.txt file is created using PROGRAM to see msg communication

Contains MSG log transmitted during Deadlock Termination Detection
B(weight=val) ==> stands for Basic MSG
C(weight=val) ==> Stand for Control MSG
