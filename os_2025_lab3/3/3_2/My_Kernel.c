#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <asm/current.h>
#define procfs_name "Mythread_info"
#define BUFSIZE  1024
#define MSG_SIZE    64
static char  g_msg[2][MSG_SIZE];
static pid_t g_pid[2];
static pid_t g_tid[2];
static u64   g_time_ms[2];
static char  g_out[2][BUFSIZE];
static DEFINE_MUTEX(g_lock);
/* Helper: choose slot (0/1) */
static inline int slot_idx(void)
{
    /* For this lab, 2 threads. Use pid parity as slot. */
    return (int)(current->pid & 1);
}
static ssize_t Mywrite(struct file *fileptr,
                       const char __user *ubuf,
                       size_t buffer_len,
                       loff_t *offset)
{
    size_t len;
    int idx;
    pid_t pid, tid;
    u64 time_ms;

    if (!ubuf || buffer_len == 0)
        return 0;

    idx = slot_idx();
    pid = current->tgid;  /* process id */
    tid = current->pid;   /* thread id */

    /* Slide-style time (rough): user time -> ms */
    time_ms = (u64)current->utime / 100 / 1000;

    len = buffer_len;
    if (len >= MSG_SIZE) len = MSG_SIZE - 1;

    mutex_lock(&g_lock);

    memset(g_msg[idx], 0, MSG_SIZE);
    if (copy_from_user(g_msg[idx], ubuf, len)) {
        mutex_unlock(&g_lock);
        return -EFAULT;
    }
    g_msg[idx][len] = '\0';

    g_pid[idx] = pid;
    g_tid[idx] = tid;
    g_time_ms[idx] = time_ms;

    mutex_unlock(&g_lock);

    return (ssize_t)len;
}


static ssize_t Myread(struct file *fileptr,
                      char __user *ubuf,
                      size_t buffer_len,
                      loff_t *offset)
{
    int idx;
    size_t out_len = 0;

    if (!ubuf)
        return -EINVAL;

    /* procfs EOF rule: only return data once per open */
    if (*offset > 0)
        return 0;

    idx = slot_idx();

    mutex_lock(&g_lock);

    /* If nothing written for this slot yet, EOF */
    if (g_msg[idx][0] == '\0') {
        mutex_unlock(&g_lock);
        return 0;
    }

    /* Build output into per-slot output buffer */
    memset(g_out[idx], 0, BUFSIZE);

    out_len += scnprintf(g_out[idx] + out_len, BUFSIZE - out_len,
                         "%s\n", g_msg[idx]);

    out_len += scnprintf(g_out[idx] + out_len, BUFSIZE - out_len,
                         "PID: %d, TID: %d, time: %llu\n",
                         g_pid[idx], g_tid[idx],
                         (unsigned long long)g_time_ms[idx]);

    mutex_unlock(&g_lock);

    /* respect user buffer size */
    if (out_len > buffer_len)
        out_len = buffer_len;

    if (copy_to_user(ubuf, g_out[idx], out_len))
        return -EFAULT;

    *offset += out_len;
    return (ssize_t)out_len;
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
