//
// Created by Md. Marufur Rahman on 25/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MSG_KEY 1234
struct message {
    long msg_type;
    int source_id;
    int seq_n;
    char data[40];
};

int main() {
    struct message msg;
    msg.msg_type = 99;
    msg.seq_n = 0;
    msg.source_id = 0;

   while (1) {
       char header[60] = "HACKER Ja ja ja... ";
       int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
       strncpy(msg.data, header, sizeof(msg.data) - 1); // Copy string safely
       msg.data[sizeof(msg.data) - 1] = '\0';           // Ensure null termination

       if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
           perror("Process: msgsnd");
           exit(1);
       }

       usleep(rand()%100 * 1e5);
   }

}
