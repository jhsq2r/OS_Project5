#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

//Creator: Jarod Stagner
//Turn-in date:

#define SHMKEY 55555
#define PERMS 0644
#define MSGKEY 66666

typedef struct msgbuffer {
        long mtype;
        int intData;//may need to add another in for pid, may not be needed though
} msgbuffer;

static void myhandler(int s){
        printf("Killing all... exiting...\n");
        kill(0,SIGTERM);

}

static int setupinterrupt(void) {
                struct sigaction act;
                act.sa_handler = myhandler;
                act.sa_flags = 0;
                return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL));
}
static int setupitimer(void) {
                struct itimerval value;
                value.it_interval.tv_sec = 60;
                value.it_interval.tv_usec = 0;
                value.it_value = value.it_interval;
                return (setitimer(ITIMER_PROF, &value, NULL));
}

struct PCB {
        int occupied;
        pid_t pid;
        int startSeconds;
        int startNano;
};

void displayTable(int i, struct PCB *processTable, FILE *file){
        fprintf(file,"Process Table:\nEntry Occupied PID StartS StartN\n");
        printf("Process Table:\nEntry Occupied PID StartS StartN\n");
        for (int x = 0; x < i; x++){
                fprintf(file,"%d        %d      %d      %d      %d\n",x,processTable[x].occupied,processTable[x].pid,processTable[x].startSeconds,processTable[x].startNano);
                printf("%d      %d      %d      %d      %d\n", x,processTable[x].occupied,processTable[x].pid,processTable[x].startSeconds,processTable[x].startNano);

        }
}

void updateTime(int *sharedTime){
        sharedTime[1] = sharedTime[1] + 100000000;
        if (sharedTime[1] >= 1000000000 ){
                sharedTime[0] = sharedTime[0] + 1;
                sharedTime[1] = sharedTime[1] - 1000000000;
        }
}

//may need custom data structures for resource management

int main(int argc, char** argv) {

        msgbuffer messenger;
        msgbuffer receiver;
        int msqid;

        if ((msqid = msgget(MSGKEY, PERMS | IPC_CREAT)) == -1) {
                perror("msgget in parent");
                exit(1);
        }

        if (setupinterrupt() == -1) {
                perror("Failed to set up handler for SIGPROF");
                return 1;
        }
        if (setupitimer() == -1) {
                perror("Failed to set up the ITIMER_PROF interval timer");
                return 1;
        }

        srand(time(NULL));
        int seed = rand();
        int proc = 5;
        int simul = 3;
        int maxTime = 50000000;//default parameters
        FILE *file;

        int shmid = shmget(SHMKEY, sizeof(int)*2, 0777 | IPC_CREAT);
        if(shmid == -1){
                printf("Error in shmget\n");
                return EXIT_FAILURE;
        }
        int * sharedTime = (int *) (shmat (shmid, 0, 0));
        sharedTime[0] = 0;
        sharedTime[1] = 0;

        int option;
        while((option = getopt(argc, argv, "hn:s:t:f:")) != -1) {//Read command line arguments
                switch(option){
                        case 'h':
                                help();
                                return EXIT_FAILURE;
                                break;
                        case 'n':
                                proc = atoi(optarg);
                                break;
                        case 's':
                                simul = atoi(optarg);
                                break;
                        case 't':
                                maxTime = atoi(optarg);
                                break;
                        case 'f':
                                file = fopen(optarg,"w");
                                break;
                        case '?':
                                help();
                                return EXIT_FAILURE;
                                break;
                }
        }

        fprintf(file,"Ran with arguments -n %d -s %d -t %d \n", proc,simul,maxTime);


        struct PCB processTable[20];
        for (int y = 0; y < 20; y++){
                processTable[y].occupied = 0;
        }

        int totalInSystem = 0;
        int status;
        int totalLaunched = 0;
        int next = 0;
        int nextLaunchTime[2];
        nextLaunchTime[0] = 0;
        nextLaunchTime[1] = 0;
        int canLaunch;
        while(1){
                seed++;
                srand(seed);

                //increment time grab function from project4, function may need editing in terms of time increment
                updateTime(sharedTime);
                
                //check if process has terminated 
                for (int x = 0; x < totalLaunched; x++){
                        if (processTable[x].occupied == 1){
                                if (waitpid(processTable[x].pid, &status, 0) > 0){
                                        processTable[x].occupied = 0;
                                        //free resources
                                        //Edit charts as needed
                                }
                        }
                }
                //recalculate total
                totalInSystem = 0;
                for (int x = 0; x < proc; x++){
                        totalInSystem += processTable[x].occupied;
                }
                
                if (totalLaunched != 0){//see if worker can be launched
                        if (nextLaunchTime[0] < sharedTime[0]){
                                canLaunch = 1;
                        }else if (nextLaunchTime[0] == sharedTime[0] && nextLaunchTime[1] <= sharedTime[1]){
                                canLaunch = 1;
                        }else{
                                canLaunch = 0;
                        }
                }
          
                //launch worker if
                if((totalInSystem < simul && canLaunch == 1 && totalLaunched < proc) || totalLaunched == 0){

                        nextLaunchTime[1] = sharedTime[1] + maxTime;
                        nextLaunchTime[0] = sharedTime[0];
                        while (nextLaunchTime[1] >= 1000000000){
                                nextLaunchTime[0] = nextLaunchTime[0] + 1;
                                nextLaunchTime[1] = nextLaunchTime[1] - 1000000000;
                        }

                        pid_t child_pid = fork();
                        if(child_pid == 0){
                                char *args[] = {"./worker", NULL};
                                execvp("./worker", args);

                                printf("Something horrible happened...\n");
                                exit(1);
                        }else{
                                processTable[totalLaunched].occupied = 1;
                                processTable[totalLaunched].pid = child_pid;
                                processTable[totalLaunched].startSeconds = sharedTime[0];
                                processTable[totalLaunched].startNano = sharedTime[1];
                                printf("Generating process with PID %d at time %d:%d\n",child_pid,sharedTime[0],sharedTime[1]);
                                fprintf(file,"Generating process with PID %d at time %d:%d\n",child_pid,sharedTime[0],sharedTime[1]);
                        }
                        totalLaunched++;
                }
                //Dont wait for message, but check
                if (msgrcv(msqid, &receiver, sizeof(msgbuffer), getpid(),0) == -1) {//change to nonblocking
                        perror("failed to receive message in parent\n");
                        exit(1);
                }
                //if you get a message
                      //if a request to this
                      //if a release do this

                //every half second, output resource table and PCB, maybe the other matrix's too
                if (sharedTime[1] == 500000000 || sharedTime[1] == 0){//This may need editing too
                        displayTable(i, processTable, file);
                }
                //every second, check for deadlock

                //if deadlock do this
        }

        displayTable(proc, processTable, file);
        //display resource, allocation array
        //display allocation matrix, request matrix

        //display stats

        shmdt(sharedTime);
        shmctl(shmid,IPC_RMID,NULL);
        if (msgctl(msqid, IPC_RMID, NULL) == -1) {
                perror("msgctl to get rid of queue in parent failed");
                exit(1);
        }

        printf("Done\n");
        fprintf(file, "Done\n");
        fclose(file);

        return 0;
}
