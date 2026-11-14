
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
	$K/initcode.o

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

$K/kernel: $(OBJS) $K/kernel.ld $K/initcode.c
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$K/kernel.bin: $K/kernel
	$(OBJCOPY) -O binary -S $< $@

$K/%.o: $K/%.S
	$(CC) -march=rv64gc -g -c -o $@ $<

$K/initcode.c: $U/initcode
	$U/gen_ccode.sh $U/initcode $K/initcode.c

$U/start.o: $U/start.S
	$(CC) $(CFLAGS) -c -o $U/start.o $U/start.S

ULIB = $U/ulib.o $U/usys.o $U/printf.o

$U/initcode: $U/start.o $U/init.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e _start -Ttext 0 -o $U/initcode.out $U/start.o $U/init.o $(ULIB)
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.out > $U/initcode.asm

$U/usys.S: $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o: $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

-include kernel/*.d user/*.d

.PHONY: clean
clean:
	rm -f $K/*.o $K/*.d $K/*.asm $K/*.sym \
	      $K/kernel $K/kernel.bin $K/initcode.c \
	      $U/*.o $U/*.d $U/*.asm \
	      $U/initcode $U/initcode.out $U/usys.S
