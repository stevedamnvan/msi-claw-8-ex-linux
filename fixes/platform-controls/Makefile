obj-m += msi-wmi-platform.o

KERNELRELEASE ?= $(shell uname -r)
KERNEL_DIR ?= /lib/modules/$(KERNELRELEASE)/build
KERNEL_COMPILER := $(shell sed -n 's/^\#define LINUX_COMPILER[[:space:]]*"\(.*\)"/\1/p' \
	$(KERNEL_DIR)/include/generated/compile.h 2>/dev/null)

ifneq ($(findstring clang,$(KERNEL_COMPILER)),)
TOOLCHAIN_ARGS := LLVM=1
endif

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) $(TOOLCHAIN_ARGS) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) $(TOOLCHAIN_ARGS) clean
