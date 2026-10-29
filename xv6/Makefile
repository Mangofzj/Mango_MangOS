
K=kernel

OBJS = \
	$K/entry.o \
	$K/main.o \
	$K/sbi.o \
	$K/printf.o \
	$K/kalloc.o \
	$K/string.o \
	$K/proc.o \
	$K/swtch.o \
	$K/user.o \
	$K/trap.o \
	$K/kernelvec.o

TOOLPREFIX = riscv64-unknown-elf-

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -march=rv64gc
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding
CFLAGS += -fno-common -nostdlib
CFLAGS += -fno-builtin
CFLAGS += -Wno-main
CFLAGS += -I.
CFLAGS += -fno-stack-protector
CFLAGS += -fno-pie

LDFLAGS = -z max-page-size=4096

$K/kernel: $(OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$K/kernel.bin: $K/kernel
	$(OBJCOPY) -O binary -S $< $@

$K/%.o: $K/%.S
	$(CC) -march=rv64gc -g -c -o $@ $<

-include kernel/*.d

.PHONY: clean
clean:
	rm -f $K/*.o $K/*.d $K/*.asm $K/*.sym \
	      $K/kernel $K/kernel.bin
