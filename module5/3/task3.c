#include <linux/module.h>
#include <linux/configfs.h>
#include <linux/init.h>
#include <linux/tty.h>          /* For fg_console, MAX_NR_CONSOLES */
#include <linux/kd.h>           /* For KDSETLED */
#include <linux/vt.h>
#include <linux/console_struct.h>       /* For vc_cons */
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>


MODULE_DESCRIPTION("Example module illustrating the use of Keyboard LEDs.");
MODULE_LICENSE("GPL");

struct timer_list my_timer;
struct tty_driver *my_driver;

static int _kbledstatus = 0;
static int test = 7;
static int blink_enabled = 1;

static struct kobject *kbleds_kobj;

#define BLINK_DELAY   (HZ/5)
#define ALL_LEDS_ON   0x07
#define RESTORE_LEDS  0xFF

static void my_timer_func(struct timer_list *ptr)
{
        int *pstatus = &_kbledstatus;
        if (*pstatus == test)
                *pstatus = RESTORE_LEDS;
        else
                *pstatus = test;
        (my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,
                            *pstatus);
        my_timer.expires = jiffies + BLINK_DELAY;
        add_timer(&my_timer);
}

static ssize_t blink_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf)
{
        return sprintf(buf, "%d\n", blink_enabled);
}

static ssize_t blink_store(struct kobject *kobj, struct kobj_attribute *attr,
                            const char *buf, size_t count)
{
        int val;

        if (kstrtoint(buf, 10, &val) < 0)
                return -EINVAL;

        if (val) {
                if (!blink_enabled) {
                        blink_enabled = 1;
                        my_timer.expires = jiffies + BLINK_DELAY;
                        add_timer(&my_timer);
                }
        } else {
                if (blink_enabled) {
                        blink_enabled = 0;
                        timer_delete_sync(&my_timer);
                        (my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty,
                                            KDSETLED, RESTORE_LEDS);
                }
        }
        return count;
}

static struct kobj_attribute blink_attribute =
        __ATTR(blink, 0664, blink_show, blink_store);

static int __init kbleds_init(void)
{
        int i;
        int retval;

        printk(KERN_INFO "kbleds: loading\n");
        printk(KERN_INFO "kbleds: fgconsole is %x\n", fg_console);
        for (i = 0; i < MAX_NR_CONSOLES; i++) {
                if (!vc_cons[i].d)
                        break;
                printk(KERN_INFO "poet_atkm: console[%i/%i] #%i, tty %lx\n", i,
                       MAX_NR_CONSOLES, vc_cons[i].d->vc_num,
                       (unsigned long)vc_cons[i].d->port.tty);
        }
        printk(KERN_INFO "kbleds: finished scanning consoles\n");
        my_driver = vc_cons[fg_console].d->port.tty->driver;

        kbleds_kobj = kobject_create_and_add("kbleds", kernel_kobj);
        if (!kbleds_kobj)
                return -ENOMEM;

        retval = sysfs_create_file(kbleds_kobj, &blink_attribute.attr);
        if (retval) {
                kobject_put(kbleds_kobj);
                return retval;
        }

        timer_setup(&my_timer, my_timer_func, 0);

        my_timer.expires = jiffies + BLINK_DELAY;

        add_timer(&my_timer);

        return 0;
}
static void __exit kbleds_cleanup(void)
{
        printk(KERN_INFO "kbleds: unloading...\n");
        kobject_put(kbleds_kobj);
        if (blink_enabled)
                timer_shutdown_sync(&my_timer);
        (my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,
                            RESTORE_LEDS);
}
module_init(kbleds_init);
module_exit(kbleds_cleanup);