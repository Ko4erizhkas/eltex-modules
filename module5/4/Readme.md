### Написать модуль ядра для своей версии ядра, который будет обмениваться информацией с userspace через chardev. 

### Сборка 
**P.s - сборку и удаление необходимо проводить в директории /eltex-module/module5/4**


**Сборка**
```bash 
make

sudo mknod /dev/chardev c <MAJOR> 0
sudo chmod 666 /dev/chardev
```
"Major" можно узнать написав и найти сообщение по типу "I was assigned major number ..." где в конце будет указано число.

```bash
sudo dmesg | tail | grep "major number"
```

**Удаление**
```bash
make clean
sudo rm -f /dev/chardev 
```



