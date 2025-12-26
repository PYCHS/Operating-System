#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/current.h>

#define procfs_name "Mythread_info"
#define BUFSIZE  1024
char buf[BUFSIZE]; //kernel buffer

static ssize_t Mywrite(struct file *fileptr, const char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    #define STATE_BASE 0
    #define OUT_BASE   256
    #define MSG_SIZE   64
    #define META_BASE  (STATE_BASE + 2 * MSG_SIZE)  /* 128 */
    #define META_SIZE  16                           /* pid(4) + tid(4) + time(8) */

    size_t len;
    int idx;
    pid_t pid = current->tgid;   /* process id */
    pid_t tid = current->pid;    /* thread id */
    u64 time_ms = (u64)current->utime / 100 / 1000;  /* follow slide style */

    idx = (int)(tid & 1);

    /* copy message from user into per-thread message slot */
    len = buffer_len;
    if (len >= MSG_SIZE) len = MSG_SIZE - 1;

    if (copy_from_user(buf + STATE_BASE + idx * MSG_SIZE, ubuf, len))
        return -EFAULT;

    buf[STATE_BASE + idx * MSG_SIZE + len] = '\0';

    /* store metadata: PID, TID, time_ms */
    memcpy(buf + META_BASE + idx * META_SIZE + 0, &pid, sizeof(pid));
    memcpy(buf + META_BASE + idx * META_SIZE + 4, &tid, sizeof(tid));
    memcpy(buf + META_BASE + idx * META_SIZE + 8, &time_ms, sizeof(time_ms));

    return buffer_len;
    /****************/
}


static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    #define STATE_BASE 0
    #define OUT_BASE   256
    #define MSG_SIZE   64
    #define META_BASE  (STATE_BASE + 2 * MSG_SIZE)
    #define META_SIZE  16

    int idx;
    int out_len = 0;
    pid_t pid, tid;
    u64 time_ms;
    char *out = buf + OUT_BASE;
    char *msg;

    /* proc read EOF rule */
    if (*offset > 0)
        return 0;

    tid = current->pid;
    idx = (int)(tid & 1);

    msg = buf + STATE_BASE + idx * MSG_SIZE;
    if (msg[0] == '\0')
        return 0;

    /* load metadata */
    memcpy(&pid,     buf + META_BASE + idx * META_SIZE + 0, sizeof(pid));
    memcpy(&tid,     buf + META_BASE + idx * META_SIZE + 4, sizeof(tid));
    memcpy(&time_ms, buf + META_BASE + idx * META_SIZE + 8, sizeof(time_ms));

    /* format output (2 lines like the slide) */
    out_len += scnprintf(out + out_len, BUFSIZE - OUT_BASE - out_len, "%s\n", msg);
    out_len += scnprintf(out + out_len, BUFSIZE - OUT_BASE - out_len,
                        "PID: %d, TID: %d, time: %llu\n",
                        pid, tid, (unsigned long long)time_ms);

    /* respect user buffer_len */
    if ((size_t)out_len > buffer_len)
        out_len = (int)buffer_len;

    if (copy_to_user(ubuf, out, out_len))
        return -EFAULT;

    *offset += out_len;
    return out_len;
    /****************/
}

static struct proc_ops Myops = {
    .proc_read = Myread,
    .proc_write = Mywrite,
};

static int My_Kernel_Init(void){
    proc_create(procfs_name, 0644, NULL, &Myops);   
    pr_info("My kernel says Hi");
    return 0;
}

static void My_Kernel_Exit(void){
    pr_info("My kernel says GOODBYE");
}

module_init(My_Kernel_Init);
module_exit(My_Kernel_Exit);

MODULE_LICENSE("GPL");
