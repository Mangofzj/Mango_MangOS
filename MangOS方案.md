# TimerOS + xv6 综合项目执行方案

## 1. 项目目标与边界

目标是在已有的 **QuardStar QEMU 自定义板 + OpenSBI + Domain** 平台上，直接实现并移植汪辰老师课程中的 xv6-riscv；完成 xv6 的多任务、进程、虚拟内存、用户态和进程生命周期功能后，再验证 FreeRTOS 在已有 trusted domain 中运行。

最终运行结构以 TimerOS 当前 DTS 配置为准：

```text
OpenSBI Domain
├── untrusted domain：CPU0～CPU6，运行 QuardStar 版 xv6
└── trusted domain：CPU7，运行 FreeRTOS
```

本项目以 C、RISC-V、QEMU、OpenSBI、BSP 和 OS 为主线，面向 Linux 内核/驱动/BSP/系统软件方向。

### 固定约束

- TimerOS 只负责：QuardStar 板级实现、OpenSBI、Domain、FreeRTOS，以及移植时的平台侧参考。
- xv6 是唯一的 OS 架构主线；**不再从头实现一套 TimerOS OS**。
- xv6 在第二阶段能从 OpenSBI 启动后，后续章节都直接在 QuardStar 版 xv6 上实现。
- 不要求每章都在原始 QEMU virt 再做一遍；原始 xv6 提交只用于理解、比对和排错。
- xv6 课程仓库按章节线性增量提交；项目始终维护一套 QuardStar 版 xv6，不在 ch13→ch14 切换代码基线。
- 实际动手范围止于 xv6 `ch20-ramfs` 和 TimerOS 第 26 章；TimerOS 第 27 章仅作参考，不扩展完整磁盘文件系统、网络或 TimerOS 第 28 章 VirtIO。

## 2. 现有课程材料

### TimerOS

仓库：<https://gitee.com/yang_lian/quard-star>

项目目录：`/home/mango/embedded/plct/timeros/timer-os`

课件目录：`slide/`

- 第 0 章：前置知识、工具链、从源码构建 QEMU
- 第 1～10 章：QuardStar、设备、OpenSBI、Domain、SBI 控制台
- 第 11～24、27 章：TimerOS 自身 OS 实现；只作为 xv6 移植时的参考
- 第 25～26 章：QuardStar 上移植和构建 FreeRTOS

### xv6

仓库：<https://github.com/unicornx/xv6-riscv-step-by-step>

项目目录：`/home/mango/embedded/plct/xv6/xv6-riscv-step-by-step`

课件目录：`slides/`

- `ch00`～`ch13`：多任务调度部分
- `ch14`～`ch20`：进程、系统调用、RAMFS 部分
- 课程 README：`README_cn.md`

## 3. 执行总流程

```text
TimerOS 0～10：建立 QuardStar + OpenSBI + 基础 Domain
        ↓
xv6 ch00～03：让 xv6 由 OpenSBI 在 QuardStar 上启动
        ↓
xv6 ch04～13：内存、Trap、中断、调度、锁、SMP
        ↓
xv6 ch14～18：页表、用户进程、系统调用、fork/exec/wait
        ↓
xv6 ch19～20：用户堆和 RAMFS
        ↓
TimerOS 25～26：CPU7 上运行 FreeRTOS
```

每完成一个小节，均执行：

```text
看视频/PDF → 看对应 xv6 提交 → 在 QuardStar 版 xv6 中实现
→ 编译运行 → GDB 定位问题 → 自己提交 Git → 写简短学习笔记
```

只有出现无法判断的**纯软件逻辑问题**（如调度算法、页表遍历逻辑、fork/exec/wait、RAMFS）时，才临时运行原始 xv6 对应提交辅助排错；boot、Trap 权限、中断/定时器、PLIC、设备与物理内存布局必须在 QuardStar 上验证，不能用原始 QEMU virt 能运行来证明移植正确。

## 4. 阶段一：完成 QuardStar 平台、OpenSBI 和基础 Domain

严格顺序学习并实现 TimerOS 以下课件：

1. `0.做项目前必看！！！！！！！.pdf`
2. `0.riscv 编译工具链构建.pdf`
3. `0.从源码构建 Qemu.pdf`
4. `1.Qemu 中自定义开发板.pdf`
5. `2.virt 源码分析.pdf`
6. `3.为 quard-star 添加 flash.pdf`
7. `4.为 quard-star 添加中断控制器.pdf`
8. `5.为 quard-star 添加 RTC 和 UART.pdf`
9. `6.制作测试固件验证串口打印.pdf`
10. `7.为 quard-star 移植 opensbi-1-2.pdf`
11. `8.基于 opensbi 为 quard-star 创建 domain.pdf`
12. `9.基于 Opensbi 服务完成控制台输出.pdf`
13. `10.封装 printf 函数.pdf`

### 本阶段完成标准

- QuardStar QEMU、Flash、PLIC、CLINT/ACLINT、RTC、UART 都按课件存在并工作；以当前 QEMU 设备模型及 DTS（如 `clint@2000000` / `riscv,clint0`）核对具体实现，避免混用 CLINT 与 ACLINT 术语。
- OpenSBI 能启动，并按当前 `dts/quard_star_sbi.dts` 建立 Domain。
- S 模式测试程序能通过 SBI 输出。
- 能解释启动链、设备地址和 CPU0～6 / CPU7 的 Domain 分配。
- 可提前检查现有 `trusted_fw.bin` 的构建与启动情况；其已被打包不等于 FreeRTOS 已成功运行，最终共存验收仍在阶段五。

到此停止 TimerOS 自己的 OS 主线；第 11 章以后不连续照抄实现。

## 5. 阶段二：将 xv6 启动到 QuardStar

学习 xv6：

1. `ch00-introduction.pdf`
2. `ch01-setup-environment.pdf`
3. `ch02-boot.pdf`
4. `ch03-hello-xv6.pdf`

本阶段做的不是新造一套启动链，而是把 TimerOS 第 7～10 章已经建立的 OpenSBI/Domain 启动链，接到 xv6 上：untrusted domain 的下一级载荷从原 TimerOS OS 改为 xv6。

### 必须完成的适配点

- **S 模式入口**：修改 xv6 `kernel/entry.S`，hart ID 从 OpenSBI 传入的 `a0` 获取，不再读取 M 模式的 `mhartid`；建立正确的栈和入口初始化流程。移除或绕开 `kernel/start.c` 中只允许 M 模式执行的初始化，包括涉及 `mstatus`、`mepc`、`mie`、`medeleg`、`mideleg`、`mscratch`、`menvcfg`、`mcounteren` 和 PMP 的 CSR 操作及 `mret`；按实际功能使用 S 模式的 `sstatus`、`sepc`、`sie`、`sscratch` 等，而非机械逐项替换。可参考 TimerOS `os/src/entry.S` 和 `os/os.ld`，但保留 xv6 自身必要的初始化。
- **链接及内存布局**：将 xv6 `kernel/kernel.ld` 的内核起始地址和 `kernel/memlayout.h` 的 `KERNBASE` 适配到 `0x80200000`。核对 QuardStar 的 DDR、OpenSBI、DTB、载荷和 trusted domain 保留区；尤其 `0x80000000～0x80200000` 不可分配给 xv6，`PHYSTOP` 必须不高于 trusted 内存起点 `0xBF800000`，且不能把 `0x82200000` 的 DTB 等保留数据当作空闲页。实际可用区间以 DTS/Domain 权限和加载布局核对结果为准。
- **启动参数与单核验证**：OpenSBI 通常通过 `a0` 传入 hart ID、`a1` 传入下一阶段启动参数（通常为 DTB 地址）；本课程 xv6 不依赖解析 DTB，设备与内存地址以手工核对后的 `memlayout.h` 为准。应核查当前 DTS 的 `next-arg1`、实际传入的 `a1`、真正 DTB 的加载位置及需保留的内存区间，不能预先断定三者一致。阶段二先只让 CPU0 进入 xv6，其他 hart 的独立栈、启动同步和多核调度留到 ch12。
- **控制台**：阶段二最小打印可以使用 SBI Console 或直接访问 UART0；按当前板级配置核对后，QuardStar UART0 为 `0x10000000`、PLIC 为 `0x0c000000`、UART0 IRQ 为 `10`，与课程 xv6 的相关默认地址一致，优先保留 xv6 原生 UART0 MMIO 驱动，以便 ch08/ch17 的串口输入和 UART 中断通过 PLIC IRQ10 工作。
- **Flash 载荷**：沿用 QuardStar `-bios none` + pflash + lowlevelboot 的加载链，而非 xv6 原有 `-kernel` 启动法。将 xv6 ELF 通过 `objcopy -O binary` 生成裸 `bin`，替换 `build.sh` 中原 `os.bin` 的打包输入（Flash 偏移 8 MiB）；核对 lowlevelboot 将 Flash `0x20800000～0x20C00000` 复制至 DDR `0x80200000～0x80600000` 的实现，确保内核镜像不超过该 4 MiB 窗口。

定时器不作为 `Hello xv6` 的前置条件，但须在 ch09 启用时钟中断前完成适配：课程 xv6 的 `w_stimecmp()` 依赖 Sstc；本项目优先改用 `sbi_set_timer()`（参考 TimerOS `os/src/sbi.c`、`os/src/timer.c`），并实现 S 模式时钟中断使能、Trap 处理和下次定时器重设。不能仅凭 DTS 未列出 `sstc` 就断定硬件一定不支持该扩展；同时不得在 S 模式写 `menvcfg`、`mcounteren` 等 M 模式 CSR。

### 对照课件

| xv6 内容 | TimerOS 参考 |
| --- | --- |
| `ch02-boot` | 第 7～8 章：OpenSBI 与 Domain |
| `ch03-hello-xv6` | 第 5～6、9～10 章：UART、SBI 控制台、printf |

### 本阶段完成标准

```text
QuardStar → OpenSBI → untrusted domain → xv6 S 模式入口 → Hello xv6
```

- 以替换 Flash 中的 `os.bin` 为验收动作，在 QuardStar QEMU 上打印 `Hello xv6`。
- 用 GDB/CSR 检查确认已在 S 模式执行，无 M 模式 CSR 非法指令；检查实际入口为 `0x80200000`。
- 明确记录保留内存区及可分配页范围，避免后续 ch04 误分配 OpenSBI、DTB 或 trusted domain 内存。

这一步通过后，后续 xv6 功能全部直接在 QuardStar 版上继续开发。

## 6. 阶段三：完成 xv6 多任务调度部分

按以下 xv6 课件顺序直接实现：

1. `ch04-malloc.pdf`
2. `ch05-first-task.pdf`
3. `ch06-cooperative-mt.pdf`
4. `ch07-trap.pdf`
5. `ch08-interrupt.pdf`
6. `ch09-hw-timers.pdf`
7. `ch10-preemptive-mt.pdf`
8. `ch11-spinlock.pdf`
9. `ch12-smp.pdf`
10. `ch13-rvos2.pdf`

### TimerOS 参考对应表

| xv6 章节 | TimerOS 参考课件 | 使用方式 |
| --- | --- | --- |
| ch04 物理内存管理 | 第 14 章中的物理内存管理 | 按阶段二核对的保留区设置 kalloc 范围；`PHYSTOP ≤ 0xBF800000` 不代表该范围全部可用 |
| ch05～06 任务与协作调度 | 第 12 章 | 仅参考任务切换和栈布局 |
| ch07 Trap | 第 11 章 | 仅参考 Trap 机制；以 xv6 设计为准 |
| ch08 外部中断 | 第 4～5 章 | 参考 PLIC 和 UART 平台配置 |
| ch09 硬件定时器 | 第 13 章 | 将课程 xv6 的 `stimecmp` 路径优先适配为 `sbi_set_timer`，并验证 S 模式时钟中断 |
| ch10 抢占调度 | 第 13 章 | 参考时钟中断进入调度 |
| ch11 自旋锁 | 无直接对应 | 以 xv6 课件为准 |
| ch12 SMP | 第 8 章 Domain | xv6 使用 untrusted domain 中 CPU0～6；处理每 hart 栈、启动同步和 hart ID 映射；检查 CPU0～6 与 CPU7 共用 PLIC 时的 context、中断源使能与路由 |
| ch13 RVOS2 | 无直接对应 | 以 xv6 课件为准 |

### 阶段检查点

- 完成 ch10：QuardStar 上可运行抢占式多任务。
- 完成 ch13：完成 xv6 课程第一部分，具备锁、SMP 和内核态控制台。

## 7. 阶段四：完成 xv6 进程部分

按以下 xv6 课件顺序直接实现：

1. `ch14-pagetable.pdf`
2. `ch15-process.pdf`
3. `ch16-syscall.pdf`
4. `ch17-sleep.pdf`
5. `ch18-process2.pdf`

### TimerOS 参考对应表

| xv6 章节 | TimerOS 参考课件 |
| --- | --- |
| ch14 页表 | 第 15、16、17 章：内存映射、调试、MMU；确保 QuardStar UART/PLIC 等 MMIO 在 Sv39 开启后仍被正确映射 |
| ch15 用户进程 | 第 11、18、19、20 章：Trap、应用装载、用户/内核映射、地址空间调度 |
| ch16 系统调用 | 第 14 章：用户态 printf |
| ch17 sleep/read/shell | 第 21、24 章：字符读入、user-shell |
| ch18 exit/exec/fork/wait | 第 22、23、27 章 |

### 本阶段完成标准

- Sv39 内核页表与每进程用户页表均正常工作。
- 能从内核进入 U 模式运行第一个用户程序。
- 支持系统调用、sleep/wakeup、read/write。
- 支持 fork、exec、exit、wait。
- 用户态 shell 可以启动用户程序。

完成本阶段后，第一版可放简历的 xv6 核心已经完成。

## 8. 阶段五：完成已发布扩展并加入 FreeRTOS

### 8.1 xv6 当前已发布的剩余内容

直接继续实现：

1. `ch19-heap.pdf`
2. `ch20-ramfs.pdf`

完成用户堆（`sbrk`）和简单 RAMFS。这里是当前 xv6 课程已发布内容的终点。

### 8.2 加入 FreeRTOS

回到 TimerOS，学习并复现（优先检查已有代码，缺失部分再补齐）：

1. `25.为 Quard-star 移植 FreeRTOS.pdf`
2. `26.为 Quard-star 移植 FreeRTOS - 移植代码构建.pdf`

在第一阶段已经完成的 Domain 配置上，让：

```text
CPU0～CPU6：QuardStar 版 xv6
CPU7：FreeRTOS
```

### 本阶段完成标准

- xv6 在 untrusted domain 中正常运行。
- FreeRTOS 在 trusted domain 的 CPU7 上启动并运行任务（以实际运行日志/调试结果为准，不以 `trusted_fw.bin` 存在为准）。
- 两者能在同一 QuardStar QEMU 实例中同时运行，并验证 CPU7 的 PLIC 中断配置不会干扰 xv6 使用的 UART0/IRQ10。
- 能说清 OpenSBI Domain 中 hart、内存和设备权限的配置来源。
