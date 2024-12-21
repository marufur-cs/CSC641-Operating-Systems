//
// Created by Md. Marufur Rahman on 21/11/24.
//
//
// Created by Md. Marufur Rahman on 19/11/24.
//


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>


#define MSG_KEY 1234
int req = 0;
int rep = 1;

struct message {
    long msg_type;
    int source_id;
    int seq_n;
    char data[40];
};

// Define semaphore operations
struct sembuf P = {0, -1, 0}; // Wait operation (decrement semaphore)
struct sembuf V = {0, 1, 0};  // Signal operation (increment semaphore)

void binary_semaphore_init(int sem_id, int value) {
    if (semctl(sem_id, 0, SETVAL, value) == -1) {
        perror("semctl SETVAL failed");
        exit(EXIT_FAILURE);
    }
}

void critical_section(int me) {
    printf("### START OUTPUT FOR NoODE %d ###\n",me);
    for (int i = 1; i <= 5; i++) {
        printf("%d This is line %d\n", me, i);
    }
    printf("### END OUTPUT FOR NODE %d ###\n",me);
}

void send_to_server(int msgid, int me, int seq, int t, int des) {
    struct message msg;
    msg.msg_type = t;
    msg.seq_n = seq;
    msg.source_id = me;

    char header[60] = "### START OUTPUT FOR NODE ";
    char footer[60] = "### END OUTPUT FOR NODE ";
    char tem[30];
    char tc[10];

    sprintf(tem, "%d", me);
    strcat(header, tem);
    strcat(header, " ###");

    strncpy(msg.data, header, sizeof(msg.data) - 1); // Copy string safely
    msg.data[sizeof(msg.data) - 1] = '\0';           // Ensure null termination


    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Process: msgsnd");
        exit(1);
    }



    for (int i = 1; i <= 5; i++) {
        char m[30] = " This is line ";
        sprintf(tc, "%d", i);
        strcat(m, tc);
        sprintf(tem, "%d", me);
        strcat(tem, m);

        strncpy(msg.data, tem, sizeof(msg.data) - 1); // Copy string safely
        msg.data[sizeof(msg.data) - 1] = '\0';

        if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
            perror("Process: msgsnd");
            exit(1);
        }
        usleep(5*1e5);

    }
    sprintf(tem, "%d", me);
    strcat(footer, tem);
    strcat(footer, " ###");

    strncpy(msg.data, footer, sizeof(msg.data) - 1); // Copy string safely
    msg.data[sizeof(msg.data) - 1] = '\0';

    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Process: msgsnd");
        exit(1);
    }

}

void send_msg(int msgid, int me, int seq, int t, int des) {
    struct message msg;
    msg.msg_type = (des * 10) + t;
    msg.seq_n = seq;
    msg.source_id = me;
    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Process: msgsnd");
        exit(1);
    }
    if (t == req) {
        printf("Send Req--> %d\n", des);
    }
    else {
        printf("Send Rep--> %d\n", des);
    }
}


int main(int argc, char *argv[])
{
    int me = atoi(argv[1]);
    int nodes = atoi(argv[2]);
    int count = 5;
    srand(me);

    // Create a message queue
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);

    // Shared memory
    key_t key1 = me *10 + 1; // Unique key
    key_t key2 = me *10 + 2;
    key_t key3 = me *10 + 3;
    key_t key4 = me *10 + 4;
    key_t key5 = me *10 + 5;

    int shm_id1 = shmget(key1, sizeof(int), IPC_CREAT | 0666);
    int shm_id2 = shmget(key2, sizeof(int), IPC_CREAT | 0666);
    int shm_id3 = shmget(key3, sizeof(int), IPC_CREAT | 0666);
    int shm_id4 = shmget(key4, sizeof(int), IPC_CREAT | 0666);
    int shm_id5 = shmget(key5, nodes*sizeof(int), IPC_CREAT | 0666);

    int *seq_number = (int *)shmat(shm_id1, NULL, 0);
    int *highest_seq_number = (int *)shmat(shm_id2, NULL, 0);
    int *outstanding_reply = (int *)shmat(shm_id3, NULL, 0);
    int *request_cs = (int *)shmat(shm_id4, NULL, 0);
    int *reply_deffered = (int *)shmat(shm_id5, NULL, 0);

    if (reply_deffered == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    for (int i = 0; i < nodes; i++) {
        reply_deffered[i] = 0;
    }

    *seq_number = 0;
    *highest_seq_number = 0;
    *outstanding_reply = 0;

    int sem_id = semget(me, 1, IPC_CREAT | 0666);
    binary_semaphore_init(sem_id, 1); // Initialize semaphore to 1


    pid_t pid,pid1;
    pid = fork();
    if (pid ==0) {
        while(1){

                struct message msg;
                int t = (me * 10) + req;
                int defer = 0;
                if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), t, 0) == -1) {
                    perror("Process: msgrcv");
                    exit(EXIT_FAILURE);
                }
                *highest_seq_number = (*highest_seq_number > msg.seq_n) ?  *highest_seq_number : msg.seq_n;
                semop(sem_id, &P, 1);///////
                    printf("Received Req --> %d\n", msg.source_id);
                    // printf("%d\n", *outstanding_reply);
                    if ((*request_cs && (msg.seq_n > *seq_number )) || ((msg.source_id > me) && (msg.seq_n == *seq_number))) {
                        defer = 1;
                    }
                semop(sem_id, &V, 1);////////
                if (defer) {
                    reply_deffered[msg.source_id-1] = 1;
                    printf("Defered %d\n", msg.source_id);
                }
                else {
                    send_msg(msgid, me, 0, rep, msg.source_id);
                }
        }
    }
    else {
        pid1 = fork();
        if (pid1 ==0) {
            while(1) {
                struct message msg;
                int t = (me * 10) + rep;
                if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), t, 0) == -1) {
                    perror("Process: msgrcv");
                    exit(EXIT_FAILURE);
                }
                semop(sem_id, &P, 1);////// Wait (P operation)
                    printf("Received Rep--> %d\n",msg.source_id);
                    // printf("%d\n", *outstanding_reply);
                    *outstanding_reply -= 1;
                semop(sem_id, &V, 1);////// Signal (V operation)
            }
        }
        else {
            while(1) {
                // Wait (P operation)
                semop(sem_id, &P, 1);

                printf("Node-%d(Main)\n",me);

                *seq_number= *highest_seq_number + 1;
                *request_cs = 1;
                *outstanding_reply = nodes-1;

                semop(sem_id, &V, 1);//////// Signal (V operation)

                for (int j = 1; j<=nodes;j++) {
                    if (j!=me) {
                        send_msg(msgid, me, *seq_number,req,j);//Send Request
                    }
                }
                while (*outstanding_reply>0) {/////// Wait for reply
                }
                // critical_section(me);
                send_to_server(msgid, me, 0, 99, 0);
                *request_cs = 0;
                for (int i = 1; i<=nodes;i++) {
                    if (i!=me) {
                        if (reply_deffered[i-1]) {
                            reply_deffered[i-1] = 0;
                            send_msg(msgid, me, 0,rep,i);
                        }
                    }
                }
                usleep(rand()%500 * 1e4);

            }
        }
    }

    return 0;
}