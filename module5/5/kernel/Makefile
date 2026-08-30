obj-m += task5.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules
	sudo insmod task5.ko
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean
	sudo rm -f /dev/chardev
	sudo rmmod task5.ko
