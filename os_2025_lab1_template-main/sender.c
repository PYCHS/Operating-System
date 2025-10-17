#include "sender.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MQ_KEY   0x11C0DE
#define SHM_KEY  0x22C0DE
#define SEM_TX   "/tx_sem"
#define SEM_RX   "/rx_sem"

static sem_t *sem_tx = NULL;
static sem_t *sem_rx = NULL;
static double total_sec = 0.0;
static struct timespec t0, t1;

static inline double elapsed_sec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) * 1e-9;
}

/* ? 自動清除舊的 IPC 殘值 */
static void cleanup_ipc() {
    sem_unlink(SEM_TX);
    sem_unlink(SEM_RX);

    int qid = msgget(MQ_KEY, 0666);
    if (qid != -1) {
        msgctl(qid, IPC_RMID, NULL);
        printf("[Cleanup] Removed old message queue (key=0x%x)\n", MQ_KEY);
    }

    int shmid = shmget(SHM_KEY, sizeof(message_t), 0666);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
        printf("[Cleanup] Removed old shared memory (key=0x%x)\n", SHM_KEY);
    }

    unlink("/dev/shm/sem.tx_sem");
    unlink("/dev/shm/sem.rx_sem");
}

void send(message_t msg, mailbox_t *mb)
{
    sem_wait(sem_tx);
    clock_gettime(CLOCK_MONOTONIC, &t0);

    if (mb->flag == MSG_PASSING) {
        if (msgsnd(mb->storage.msqid, &msg, sizeof(msg.msgText), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
    } else if (mb->flag == SHARED_MEM) {
        strncpy(mb->storage.shm_addr, msg.msgText, sizeof(msg.msgText) - 1);
        mb->storage.shm_addr[sizeof(msg.msgText) - 1] = '\0';
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    total_sec += elapsed_sec(t0, t1);

    // ? 只在不是 EOF 時印出訊息
    if (strcmp(msg.msgText, "EOF") != 0 && strcmp(msg.msgText, "EOF\n") != 0) {
        printf("Sending message: %s", msg.msgText);
    }

    sem_post(sem_rx);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: ./sender <mode> <input.txt>\n");
        return 1;
    }

    mailbox_t box;
    box.flag = atoi(argv[1]);
    const char *path = argv[2];

    if (box.flag == MSG_PASSING)
        puts("Message Passing");
    else if (box.flag == SHARED_MEM)
        puts("Shared Memory");
    else {
        fprintf(stderr, "Invalid mode. Use 1 or 2.\n");
        return 1;
    }

    cleanup_ipc();

    sem_tx = sem_open(SEM_TX, O_CREAT | O_EXCL, 0644, 1);
    sem_rx = sem_open(SEM_RX, O_CREAT | O_EXCL, 0644, 0);
    if (sem_tx == SEM_FAILED || sem_rx == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    if (box.flag == MSG_PASSING) {
        int qid = msgget(MQ_KEY, 0666 | IPC_CREAT);
        if (qid == -1) {
            perror("msgget");
            return 1;
        }
        box.storage.msqid = qid;
    } else if (box.flag == SHARED_MEM) {
        int shmid = shmget(SHM_KEY, sizeof(message_t), 0666 | IPC_CREAT);
        if (shmid == -1) {
            perror("shmget");
            return 1;
        }
        box.storage.shm_addr = (char *)shmat(shmid, NULL, 0);
        if (box.storage.shm_addr == (char *)-1) {
            perror("shmat");
            return 1;
        }
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    message_t msg;
    size_t n_lines = 0;
    while (fgets(msg.msgText, sizeof(msg.msgText), fp)) {
        msg.mType = 1;
        send(msg, &box);
        n_lines++;
    }

    strcpy(msg.msgText, "EOF");
    msg.mType = 1;
    send(msg, &box);

    // printf("\n[Sender] lines(incl. EOF)=%zu\n", n_lines + 1);
    printf("\nEnd of input file! exit!\n");
    printf("total IPC time(s) taken in sending msg = %.6f\n", total_sec);

    fclose(fp);

    sem_close(sem_tx);
    sem_close(sem_rx);

    if (box.flag == SHARED_MEM) {
        shmdt(box.storage.shm_addr);
    }

    return 0;
}
