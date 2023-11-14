#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>

#define SHMKEY 55555
#define PERMS 0644
#define MSGKEY 66666

typedef struct msgbuffer {
        long mtype;
        int intData;
} msgbuffer;

int main(int argc, char** argv){

        msgbuffer receiver;
        receiver.mtype = 1;
        int msqid = 0;

        if ((msqid = msgget(MSGKEY, PERMS)) == -1) {
                perror("msgget in child");
                exit(1);
        }

        int shmid = shmget(SHMKEY, sizeof(int)*2, 0777);
        if(shmid == -1){
                printf("Error in shmget\n");
                return EXIT_FAILURE;
        }
        int * sharedTime = (int*) (shmat (shmid, 0, 0));

        //printf("This is Child: %d, From Parent: %d, Seconds: %s, NanoSeconds: %s\n", getpid(), getppid(), argv[1], argv[2]);

        //printf("WORKER PID:%d PPID:%d SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d JUST STARTING\n",getpid(),getppid(),sharedTime[0],sharedTime[1],exitTime[0],exitTime[1]);

        srand(time(NULL));
        int seed = rand();
        int percentChance;
        int givenTime;
        int myResources[10];
        for (int x = 0; x < 10; x++){
                myResources[x] = 0;
        }
        while (1){
                seed++;
                
                givenTime = receiver.intData;
                srand(seed*getpid());
                percentChance = (rand() % 100) + 1;
                //printf("PercentChance: %d\n", percentChance);
                if (percentChance <= 2){//terminate
                        break;
                }else if (percentChance > 2 && percentChance < 90){//request resource
                        receiver.intData = 1;
                        //Random number between 0 and 9
                        //Only request if we don't have 20 of that resource
                }else if (percentChance >= 90 && percentChance <= 100){//release resource
                        //Random number between 0 and 9, if myResource[rand] == 0 then increment until a resource is found
                        //if increment goes all the way back to the random number, make it known that nothing was released
                        receiver.intData = 2;
                }else{
                        printf("Something went wrong: Random number %d\n", percentChance);
                        receiver.intData = 4;
                        break;
                }

                //send message
                receiver.mtype = getppid();
                if (msgsnd(msqid, &receiver, sizeof(msgbuffer)-sizeof(long),0) == -1){
                        perror("msgsnd to parent failed\n");
                        exit(1);
                }

                //wait for message back
                if ( msgrcv(msqid, &receiver, sizeof(msgbuffer), getpid(), 0) == -1) {
                        perror("failed to receive message from parent\n");
                        exit(1);
                }
                //printf("WORKER PID:%d Message received\n",getpid());

                //if percentChance >= 90 && percentChance <= 100
                        //update myResource for the released resources
                //if percentChance < 90 && percentChance > 2
                        //update myResource for the gained resources

        }
  
        //printf("Killing child %d\n", getpid());
        //printf("WORKER PID:%d PPID:%d SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d TERMINATING\n",getpid(),getppid(),sharedTime[0],sharedTime[1],exitTime[0],exitTime[1]);
        shmdt(sharedTime);

        return 0;

}