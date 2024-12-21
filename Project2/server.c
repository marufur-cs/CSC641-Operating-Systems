//
// Created by Md. Marufur Rahman on 25/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MSG_KEY 1234

struct message {
    long msg_type;
    int source_id;
    int seq_n;
    char data[40];

};

int main(int argc, char *argv[]) {
    // Create a message queue
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    int n=0;

        while(1){
                struct message msg;
                int t = 99;
                if (msgrcv(msgid, &msg, 2*sizeof(int) + 40*sizeof(char), t, 0) == -1) {
                    perror("Process: msgrcv");
                    exit(EXIT_FAILURE);
                }
                printf("%s\n", msg.data);

        }

    return 0;

}