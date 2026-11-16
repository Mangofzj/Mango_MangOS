
K=kernel
U=user

OBJS = \
	$K/entry.o \
	$K/main.o \
	$K/sbi.o \
	$K/printf.o \
	$K/kalloc.o \
	$K/string.o \
	$K/proc.o \
	$K/swtch.o \
	$K/trap.o \
	$K/sysproc.o \
	$K/sysfile.o \
	$K/syscall.o \
	$K/uart.o \
	$K/timer.o \
	$K/kernelvec.o \
	$K/plic.o \
	$K/spinlock.o \
	$K/console.o \
	$K/vm.o \
	$K/trampoline.o \
	$K/exec.o \
	$K/_init.o

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

$K/kernel: $U/_init $(OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$K/kernel.bin: $K/kernel
	$(OBJCOPY) -O binary -S $< $@

$K/%.o: $K/%.S
	$(CC) -march=rv64gc -g -c -o $@ $<

ULIB = $U/ulib.o $U/usys.o $U/printf.o

_%: %.o $(ULIB) $U/user.ld
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $< $(ULIB)
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym
	$U/gen_ccode.sh $@ $@.c
	mv $@.c $K/

$K/_init.o: $K/_init.c
	$(CC) $(CFLAGS) -c -o $@ $<

$U/usys.S: $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o: $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

.PRECIOUS: %.o

UPROGS=\
	$U/_init\

-include kernel/*.d user/*.d

.PHONY: clean
clean:
	rm -f $K/*.o $K/*.d $K/*.asm $K/*.sym \
	      $K/kernel $K/kernel.bin \
	      $K/_*.c \
	      $U/*.o $U/*.d $U/*.asm $U/*.sym \
	      $U/usys.S \
	      $(UPROGS)
