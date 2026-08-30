obj-m += task4.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod task4.ko
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rm -f /dev/chardev
	sudo rmmod task4.ko
