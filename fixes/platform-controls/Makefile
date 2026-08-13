obj-m += msi-wmi-platform.o

KERNELRELEASE ?= $(shell uname -r)
KERNEL_DIR ?= /lib/modules/$(KERNELRELEASE)/build

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) LLVM=1 modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) LLVM=1 clean
