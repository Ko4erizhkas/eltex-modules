/*
 *  task4.c: Создает символьное устройство, которое обменивается сообщением
 *  с userspace: то, что записали через write(), возвращается через read().
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>      /* определение функций copy_to_user/copy_from_user */

/*
 *  Прототипы функций, обычно их выносят в заголовочный файл (.h)
 */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Task 4 from module 5");

static int __init chardev_init(void);
static void __exit chardev_exit(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"   /* Имя устройства, будет отображаться в /proc/devices   */
#define BUF_LEN 80                      /* Максимальная длина сообщения */

/*
 * Глобальные переменные, объявлены как static, воизбежание конфликтов имен.
 */

static int Major;             /* Старший номер устройства нашего драйвера */
static int Device_Open = 0;   /* Устройство открыто?
                               * используется для предотвращения одновременного
                               * обращения из нескольких процессов */
static char msg[BUF_LEN];     /* Здесь хранится сообщение, полученное от userspace */
static char *msg_Ptr;

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

/*
 * Функции
 */

static int __init chardev_init(void)
{
  Major = register_chrdev(0, DEVICE_NAME, &fops);

  if (Major < 0) {
    printk(KERN_ALERT "Registering the character device failed with %d\n",
            Major);
    return Major;
  }

  printk(KERN_INFO "I was assigned major number %d.  To talk to\n", Major);
  printk(KERN_INFO "the driver, create a dev file with\n");
  printk(KERN_INFO "'mknod /dev/chardev c %d 0'.\n", Major);
  printk(KERN_INFO "Try to cat and echo to the device file.\n");
  printk(KERN_INFO "Remove the device file and module when done.\n");

  return 0;
}

static void __exit chardev_exit(void)
{
  /*
   * Отключение устройства
   */
  unregister_chrdev(Major, DEVICE_NAME);
}

/*
 * Обработчики
 */

/*
 * Вызывается, когда процесс пытается открыть файл устройства, например командой
 * "cat /dev/chardev"
 */
static int device_open(struct inode *inode, struct file *file)
{
  if (Device_Open)
    return -EBUSY;
  Device_Open++;
  try_module_get(THIS_MODULE);

  return SUCCESS;
}

/*
 * Вызывается, когда процесс закрывает файл устройства.
 */
static int device_release(struct inode *inode, struct file *file)
{
  Device_Open--;                /* Теперь мы готовы обслужить другой процесс */

  /*
   * Уменьшить счетчик обращений, иначе, после первой же удачной попытки открыть файл устройства,
   * вы никогда не сможете выгрузить модуль.
   */
  module_put(THIS_MODULE);

  return 0;
}

/*
 * Вызывается, когда процесс пытается прочитать уже открытый файл устройства.
 * Возвращает то сообщение, которое ранее было записано в устройство через write().
 */
static ssize_t device_read(struct file *filp, /* см. include/linux/fs.h   */
         char __user *buffer,                 /* буфер, куда надо положить данные */
         size_t length,                       /* размер буфера */
         loff_t *offset)
{
  size_t msg_len = strlen(msg_Ptr);

  /*
   * Если достигли конца сообщения,
   * вернуть 0, как признак конца файла
   */
  if (*offset >= msg_len)
    return 0;

  if (length > msg_len - *offset)
    length = msg_len - *offset;

  /*
   * Буфер находится в пространстве пользователя (в сегменте данных),
   * а не в пространстве ядра, поэтому простое присваивание здесь недопустимо.
   * Для того, чтобы скопировать данные, мы используем функцию copy_to_user,
   * которая перенесет данные из пространства ядра в пространство пользователя.
   */
  if (copy_to_user(buffer, msg_Ptr + *offset, length))
    return -EFAULT;

  *offset += length;

  /*
   * В большинстве своем, функции чтения возвращают количество байт, записанных в буфер.
   */
  return length;
}

/*
 * Вызывается, когда процесс пытается записать в устройство,
 * например так: echo "hi" > /dev/chardev
 * Сохраняет присланные данные, чтобы их потом можно было прочитать через read().
 */
static ssize_t
device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
  if (len > BUF_LEN - 1)
    len = BUF_LEN - 1;

  if (copy_from_user(msg, buff, len))
    return -EFAULT;

  msg[len] = '\0';
  msg_Ptr = msg;

  return len;
}

module_init(chardev_init);
module_exit(chardev_exit);
