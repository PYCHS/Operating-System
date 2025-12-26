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
    #define MSG_SIZE   64
    #define META_BASE  (STATE_BASE + 2 * MSG_SIZE)  /* 128 */
    #define META_SIZE  16                           /* pid(4) + tid(4) + time(8) */

    size_t len;
    int idx;
    pid_t pid = current->tgid;
    pid_t tid = current->pid;
    /* keep your slide style time; it's OK */
    u64 time_ms = (u64)current->utime / 100 / 1000;

    idx = (int)(tid & 1);

    /* bound length and copy */
    len = buffer_len;
    if (len >= MSG_SIZE) len = MSG_SIZE - 1;

    /* optional: clear slot first to avoid leftover garbage */
    memset(buf + STATE_BASE + idx * MSG_SIZE, 0, MSG_SIZE);

    if (copy_from_user(buf + STATE_BASE + idx * MSG_SIZE, ubuf, len))
        return -EFAULT;

    buf[STATE_BASE + idx * MSG_SIZE + len] = '\0';

    /* store metadata */
    memcpy(buf + META_BASE + idx * META_SIZE + 0, &pid, sizeof(pid));
    memcpy(buf + META_BASE + idx * META_SIZE + 4, &tid, sizeof(tid));
    memcpy(buf + META_BASE + idx * META_SIZE + 8, &time_ms, sizeof(time_ms));

    /* return what we consumed */
    return (ssize_t)len;
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
    pid_t pid, tid;
    u64 time_ms;

    char *out = buf + OUT_BASE;
    char *msg;
    size_t out_len = 0;
    size_t cap = BUFSIZE - OUT_BASE;

    /* EOF rule */
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

    /* Ensure msg is not runaway */
    msg[MSG_SIZE - 1] = '\0';

    /* format output safely */
    if (out_len < cap) {
        out_len += scnprintf(out + out_len, cap - out_len, "%s\n", msg);
    }
    if (out_len < cap) {
        out_len += scnprintf(out + out_len, cap - out_len,
                            "PID: %d, TID: %d, time: %llu\n",
                            pid, tid, (unsigned long long)time_ms);
    }

    /* respect user buffer_len */
    if (out_len > buffer_len)
        out_len = buffer_len;

    if (copy_to_user(ubuf, out, out_len))
        return -EFAULT;

    *offset += out_len;
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
