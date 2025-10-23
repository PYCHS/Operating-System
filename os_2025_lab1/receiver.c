#include "receiver.h"
#include <errno.h>
#include <time.h>
#include <unistd.h>   // usleep
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
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

// Receiver要先打開sender開的semaphore
static sem_t* open_semaphore_with_retry(const char *name, int max_retry, int retry_ms) {
    // 沒開到就等一下再試，最多試max_retry次，每次間隔retry_ms毫秒
    for (int i = 0; i <= max_retry; ++i) {
        sem_t *h = sem_open(name, 0);
        if (h != SEM_FAILED) return h;
        if (errno != ENOENT) {
            perror("sem_open");
            break;
        }
        // 還沒被開好就等一下再試 -> sleep
        usleep(retry_ms * 1000);
    }
    // 全部試完還失敗回傳SEM_FAILED
    return SEM_FAILED;
}

static void receive(message_t *msg, mailbox_t *mb)
{
    // 等sender（sem_rx）
    // S--
    if (sem_wait(sem_rx) == -1) {
        // System call error
        perror("sem_wait(sem_rx)");
        exit(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &t0);

    if (mb->flag == MSG_PASSING) {
        // ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
        if (msgrcv(mb->storage.msqid, msg, sizeof(msg->msgText), 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
    }
    else if (mb->flag == SHARED_MEM) {
        // mb->storage.shm_addr->共享記憶體的起始位址
        // 把那個區塊copy進 msg->msgText
        strncpy(msg->msgText, mb->storage.shm_addr, sizeof(msg->msgText) - 1);
        msg->msgText[sizeof(msg->msgText) - 1] = '\0';
    }
    else {
        fprintf(stderr, "[Receiver] unknown mode: %d\n", mb->flag);
        exit(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    total_sec += elapsed_sec(t0, t1);

    // 告訴sender可以傳下一則
    if (sem_post(sem_tx) == -1) {
        perror("sem_post(sem_tx)");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./receiver <mode>\n");
        fprintf(stderr, "  mode: 1=MessagePassing, 2=SharedMemory\n");
        return 1;
    }

    mailbox_t box;
    box.flag = atoi(argv[1]);

    if (box.flag == MSG_PASSING)
        puts("Message Passing");
    else if (box.flag == SHARED_MEM)
        puts("Shared Memory");
    else {
        fprintf(stderr, "Invalid mode. Use 1 or 2.\n");
        return 1;
    }

    sem_tx = open_semaphore_with_retry(SEM_TX, 50, 20);
    sem_rx = open_semaphore_with_retry(SEM_RX, 50, 20);
    if (sem_tx == SEM_FAILED || sem_rx == SEM_FAILED) {
        fprintf(stderr, "sem_open failed — make sure you launched ./sender first.\n");
        return 1;
    }

    if (box.flag == MSG_PASSING) {
        int qid = msgget(MQ_KEY, 0666 | IPC_CREAT);
        if (qid == -1) {
            perror("msgget");
            return 1;
        }
        box.storage.msqid = qid;
    }
    else if (box.flag == SHARED_MEM) {
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

    // 開始receive
    size_t n_lines = 0;
    message_t msg;

    while (1) {
        receive(&msg, &box);

        // 去掉換行
        msg.msgText[strcspn(msg.msgText, "\n")] = '\0';

        // 判斷 EOF（是就跳出、不印）
        if (strcmp(msg.msgText, "EOF") == 0) break;

        // 只有非 EOF 才印
        printf("Receiving message: %s\n", msg.msgText);
        n_lines++;
    }


    printf("Sender exit!\n");
    printf("total IPC time(s) taken in receiving msg =%.6f\n", total_sec);

    // --- 清理 ---
    if (box.flag == SHARED_MEM) {
        shmdt(box.storage.shm_addr);
        int shmid = shmget(SHM_KEY, sizeof(message_t), 0666);
        if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);
    }

    sem_close(sem_tx);
    sem_close(sem_rx);
    sem_unlink(SEM_TX);
    sem_unlink(SEM_RX);

    if (box.flag == MSG_PASSING)
        msgctl(box.storage.msqid, IPC_RMID, NULL);

    return 0;
}
