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

    /* Two-slot storage (thread slot = current->pid & 1) */
    #define MSG_SIZE 64
    static char msgbuf[2][MSG_SIZE];
    static pid_t pidbuf[2];
    static pid_t tidbuf[2];
    static u64   timebuf_ms[2];

    size_t len;
    int idx;
    pid_t pid = current->tgid;  /* process id */
    pid_t tid = current->pid;   /* thread id */

    /* Follow your slide style: time in ms (rough, but OK for lab) */
    u64 time_ms = (u64)current->utime / 100 / 1000;

    idx = (int)(tid & 1);

    /* Bound copy length */
    len = buffer_len;
    if (len >= MSG_SIZE) len = MSG_SIZE - 1;

    /* Clear then copy from user */
    memset(msgbuf[idx], 0, MSG_SIZE);
    if (copy_from_user(msgbuf[idx], ubuf, len))
        return -EFAULT;

    msgbuf[idx][len] = '\0';

    /* Store metadata */
    pidbuf[idx] = pid;
    tidbuf[idx] = tid;
    timebuf_ms[idx] = time_ms;

    /* Return bytes consumed */
    return (ssize_t)len;

    /****************/
}


static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/

    #define MSG_SIZE 64
    static char msgbuf[2][MSG_SIZE];
    static pid_t pidbuf[2];
    static pid_t tidbuf[2];
    static u64   timebuf_ms[2];

    /* Two output buffers: one per slot */
    static char outbuf[2][BUFSIZE];

    int idx;
    pid_t tid;
    size_t out_len = 0;

    /* EOF rule for procfs */
    if (*offset > 0)
        return 0;

    tid = current->pid;
    idx = (int)(tid & 1);

    /* If nothing written yet for this slot, return EOF */
    msgbuf[idx][MSG_SIZE - 1] = '\0';
    if (msgbuf[idx][0] == '\0')
        return 0;

    /* Build output into per-slot out buffer (prevents smash) */
    memset(outbuf[idx], 0, BUFSIZE);

    out_len += scnprintf(outbuf[idx] + out_len, BUFSIZE - out_len, "%s\n", msgbuf[idx]);
    out_len += scnprintf(outbuf[idx] + out_len, BUFSIZE - out_len,
                         "PID: %d, TID: %d, time: %llu\n",
                         pidbuf[idx], tidbuf[idx],
                         (unsigned long long)timebuf_ms[idx]);

    /* Respect user buffer length */
    if (out_len > buffer_len)
        out_len = buffer_len;

    if (copy_to_user(ubuf, outbuf[idx], out_len))
        return -EFAULT;

    *offset += out_len;

    /*
     * Optional but recommended:
     * clear message after one successful read so user-space fgets loop ends cleanly
     */
    msgbuf[idx][0] = '\0';

    return (ssize_t)out_len;

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
