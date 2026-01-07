#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#define DEVICE_NAME "android_rw_monitor"
#define PROCFS_NAME "rw_monitor"

// 用于存储读写监控信息的环形缓冲区
#define BUFFER_SIZE 1024
static char buffer[BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

// 向环形缓冲区中添加信息
static void add_to_buffer(const char *msg) {
    int len = strlen(msg);
    if (len >= BUFFER_SIZE) {
        len = BUFFER_SIZE - 1;
    }
    for (int i = 0; i < len; i++) {
        buffer[buffer_head] = msg[i];
        buffer_head = (buffer_head + 1) % BUFFER_SIZE;
    }
}

// 读取环形缓冲区中的信息
static ssize_t read_proc(struct file *file, char __user *usr_buf, size_t count, loff_t *pos) {
    int len = 0;
    if (buffer_head == buffer_tail) {
        return 0; // 环形缓冲区为空
    }
    while (buffer_tail != buffer_head) {
        char c = buffer[buffer_tail];
        buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
        if (put_user(c, usr_buf + len)) {
            return -EFAULT;
        }
        len++;
        if (len == count) {
            break;
        }
    }
    return len;
}

// 写入环形缓冲区中的信息（用于测试）
static ssize_t write_proc(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos) {
    char msg[BUFFER_SIZE];
    if (copy_from_user(msg, usr_buf, count)) {
        return -EFAULT;
    }
    msg[count] = '\0';
    add_to_buffer(msg);
    return count;
}

// proc文件操作结构体
static const struct file_operations proc_fops = {
    .read = read_proc,
    .write = write_proc,
};

// 监控文件的读操作
static ssize_t monitor_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    printk(KERN_INFO "Read operation detected\n");
    char msg[100];
    snprintf(msg, sizeof(msg), "Read operation detected\n");
    add_to_buffer(msg);
    return 0;
}

// 监控文件的写操作
static ssize_t monitor_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    printk(KERN_INFO "Write operation detected\n");
    char msg[100];
    snprintf(msg, sizeof(msg), "Write operation detected\n");
    add_to_buffer(msg);
    return 0;
}

// 文件操作结构体
static struct file_operations fops = {
    .read = monitor_read,
    .write = monitor_write,
};

// 模块初始化函数
static int __init android_rw_monitor_init(void) {
    int ret;
    // 创建proc文件
    proc_create(PROCFS_NAME, 0666, NULL, &proc_fops);
    printk(KERN_INFO "Android read/write monitor module loaded\n");
    return 0;
}

// 模块卸载函数
static void __exit android_rw_monitor_exit(void) {
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_INFO "Android read/write monitor module unloaded\n");
}

module_init(android_rw_monitor_init);
module_exit(android_rw_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Nam");
MODULE_DESCRIPTION("A simple Android kernel read/write monitor driver");
MODULE_VERSION("0.1");
