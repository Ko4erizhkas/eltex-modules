KERNEL_DIR = kernel
USER_DIR = user

all: kernel user

kernel:
	$(MAKE) -C $(KERNEL_DIR)

user:
	$(MAKE) -C $(USER_DIR)

clean:
	$(MAKE) -C $(KERNEL_DIR) clean
	$(MAKE) -C $(USER_DIR) clean

.PHONY: all kernel user clean
