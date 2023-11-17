This program is designed to take in 5 paramters
-n [number of processes to launch]
-s [number of processes to run at one time]
-t [time interval in nanoseconds to launch processes]
-f [file to output log]
-v 1 to only see output from deadlocks
This program will fork off n processes while maintaining the limit set by s
Each process will randomly take up resources in a resource table
There are 10 resources with 20 of each
This program will check for deadlocks every simulated second
If it finds one, it will remove running processes one by one until it is no longer deadlocked
Finally, it will output some stats on the simulation

The only optional paramter is v

Repos: https://github.com/jhsq2r/OS_Project5
