### Задание 2 по модулю 5: Написать модуль ядра для своей версии ядра, который будет обмениваться информацией с userspace через proc. Адаптировать для своей версии ядра (Структура обработчиков). Избавиться от харкода (маг чисел) и изолировать переменные модуля (static).

**Взят был пример для модуля ядра 5**

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
 
int len,temp;
char *msg;
 
ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp ) {
    if(count > temp) {
        count = temp;
    }
    temp = temp - count;
    copy_to_user(buf, msg, count);
    if(count == 0)
        temp = len;
    return count;
}
 
ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp) {
    copy_from_user(msg, buf, count);
    len = count;
    temp = len;
    return count;
}
 
static const struct proc_ops proc_fops = {
    proc_read: read_proc,
    proc_write: write_proc,
};
 
void create_new_proc_entry(void) { //use of void for no arguments is compulsory now
    proc_create("proc", 0, NULL, &proc_fops);
    msg = kmalloc(10 * sizeof(char), GFP_KERNEL);
}
 
int proc_init (void) {
    create_new_proc_entry();
    return 0;
}
 
void proc_cleanup(void) {
    remove_proc_entry("hello", NULL);
    kfree(msg);
}
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fantomas");
module_init(proc_init);
module_exit(proc_cleanup);
```

**Было исправленно пару warning при компиляции модуля, а именно: Объявленны прототипы функций read_proc, write_proc, create_new_proc_entry, proc_init, proc_cleanup; Добавлена проверка для copy_from_user**

**Исправленный вариант**

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
 
#define BUF_SIZE 10
#define PROC_NAME "proc"
#define PROC_MODE 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kosyachenko Daniil");
MODULE_DESCRIPTION("Task 2 from module 5");

static int len,temp;
static char *msg;


ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp );
ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp);
void create_new_proc_entry(void);

int proc_init(void);
void proc_cleanup(void);

ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp ) {
    if(count > temp) {
        count = temp;
    }
    temp = temp - count;
    
    if (copy_to_user(buf, msg, count))
        return -EFAULT;

    if(count == 0)
        temp = len;
    return count;
}
 
ssize_t write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp) {
    if(copy_from_user(msg, buf, count))
        return -EFAULT;
    len = count;
    temp = len;
    return count;
}
 
static const struct proc_ops proc_fops = {
    proc_read: read_proc,
    proc_write: write_proc,
};
 
void create_new_proc_entry(void) { //use of void for no arguments is compulsory now
    proc_create(PROC_NAME, PROC_MODE, NULL, &proc_fops);
    msg = kmalloc(BUF_SIZE * sizeof(char), GFP_KERNEL);
}
 
int proc_init (void) {
    create_new_proc_entry();
    return 0;
}
 
void proc_cleanup(void) {
    remove_proc_entry(PROC_NAME, NULL);
    kfree(msg);
}
 

module_init(proc_init);
module_exit(proc_cleanup);
```
**Сборка проекта**

```bash
make
```
**Установка модуля ядра**

```bash
sudo insmod task2.ko
```

```bash
echo "test" | sudo tee /proc/proc
```

**Выгрузка модуля**
```bash
sudo rmmod task2.ko
```