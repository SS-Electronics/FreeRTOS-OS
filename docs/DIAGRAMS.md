# Architecture Diagrams {#architecture-diagrams}

Visual reference for every layered subsystem in FreeRTOS-OS. Every
picture on this page is rendered server-side by Doxygen using Graphviz
(`HAVE_DOT = YES` in `Doxyfile`) — the source is checked in as
`\dot ... \enddot` blocks so the diagrams stay synchronised with the
code on every `make docs` run.

Each section links to the prose document that explains the underlying
mechanism in detail.

---

## Diagram Index {#diagram-index}

| # | Topic | Source doc |
|---|---|---|
| 1 | [System Layer Stack](#system-layer-stack) | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 2 | [Repository / Build Composition](#repository--build-composition) | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 3 | [Boot Phase State Machine](#boot-phase-state-machine) | [FLOW.md](FLOW.md) |
| 4 | [Boot Function-Call Chain](#boot-function-call-chain) | [FLOW.md](FLOW.md) |
| 5 | [Thread Lifecycle](#thread-lifecycle) | [OS_THREAD.md](OS_THREAD.md) |
| 6 | [Thread Map at Steady State](#thread-map-at-steady-state) | [DEV_MGMT.md](DEV_MGMT.md) |
| 7 | [Service Thread Request / Notify Sequence](#service-thread-request--notify-sequence) | [DEV_MGMT.md](DEV_MGMT.md) |
| 8 | [Driver vtable Architecture](#driver-vtable-architecture) | [DRIVERS.md](DRIVERS.md) |
| 9 | [IRQ Flow — Hardware to Task](#irq-flow--hardware-to-task) | [IRQ.md](IRQ.md) |
| 10 | [Safety / Watchdog Loop](#safety--watchdog-loop) | [ARCHITECTURE.md#medical-grade-safety-layer](ARCHITECTURE.md#medical-grade-safety-layer) |
| 11 | [Code-Gen Pipeline (XML → C)](#code-gen-pipeline-xml--c) | [BOARD.md](BOARD.md) |

---

## System Layer Stack {#system-layer-stack}

The OS is a stack of independently-replaceable layers.  Each lower
layer exposes a vendor-agnostic API to the layer above.  Vendor / MCU
specificity is collapsed into the bottom two boxes (`arch/devices/...`
and the `drivers/hal/<vendor>/` HAL backends).

\dot
digraph layers {
    rankdir=TB;
    node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=11];
    splines=ortho;

    APP [label="Application\n(app_main.c, user tasks)", fillcolor="#FFE0B2"];

    subgraph cluster_os {
        label="FreeRTOS-OS";
        style="rounded,filled"; fillcolor="#E3F2FD"; color="#1976D2";
        fontname="Helvetica"; fontsize=12;

        SVC  [label="Service Threads\nuart_mgmt · iic_mgmt · spi_mgmt · gpio_mgmt\nadc_mgmt · os_shell_mgmt · task_mgr · wdog", fillcolor="#BBDEFB"];
        APPDRV [label="Application Driver Layer\ndrv_app/uart_app.h, iic_app.h, spi_app.h, gpio_app.h", fillcolor="#90CAF9"];
        DRV  [label="Generic Driver API\ndrv_uart · drv_iic · drv_spi · drv_gpio · drv_adc · drv_can\ndrv_dma · drv_irq · drv_rcc · drv_time · drv_cpu", fillcolor="#90CAF9"];
        HAL  [label="HAL Backends (vendor-specific)\ndrivers/hal/stm32/*.c · drivers/hal/infineon/*.c", fillcolor="#64B5F6"];
        IPC  [label="IPC · mqueue · ring buffer · printk", fillcolor="#A5D6A7"];
        IRQ  [label="IRQ subsystem\ndrv_irq · irq_desc · irq_notify · irq_chip_nvic", fillcolor="#A5D6A7"];
        MM   [label="Memory: kmalloc · kfree · sysmem · syscalls", fillcolor="#A5D6A7"];
        SAFE [label="Safety: IWDG · sw-watchdog · slog · fault_handler · safe_state · .noinit", fillcolor="#FFCDD2"];
        BOOT [label="Boot FSM (init/main.c)", fillcolor="#FFAB91"];
    }

    KERN [label="FreeRTOS Kernel\nxTaskCreate, vTaskStartScheduler, queues, notifications", fillcolor="#C8E6C9"];
    ARCH [label="arch/devices/<VENDOR>/<FAMILY>/<PART>\nCMSIS-Core · startup · linker script", fillcolor="#D7CCC8"];
    HW   [label="Physical hardware\nSTM32F411xE  /  STM32H723ZGTx", fillcolor="#BCAAA4", shape=box3d];

    APP -> APPDRV;
    APP -> SVC;
    APPDRV -> SVC -> DRV -> HAL;
    DRV -> IPC; DRV -> IRQ;
    SVC -> IPC; SVC -> SAFE;
    BOOT -> DRV; BOOT -> SAFE; BOOT -> SVC;
    DRV -> KERN; IPC -> KERN; IRQ -> KERN; MM -> KERN; SAFE -> KERN;
    KERN -> ARCH;
    HAL -> ARCH;
    ARCH -> HW;
}
\enddot

Cross-references: full prose narrative in
[`ARCHITECTURE.md` § Layer Model](ARCHITECTURE.md#layer-model).

---

## Repository / Build Composition {#repository--build-composition}

Every top-level directory has exactly one role.  `arch/` and
`drivers/hal/` are the *only* directories that contain
vendor-specific code — everything else is portable.

\dot
digraph repo {
    rankdir=LR;
    node [shape=folder, fontname="Helvetica", fontsize=10, style=filled];
    edge [arrowhead=none, color="#888"];

    ROOT [label="FreeRTOS-OS/", fillcolor="#FFF9C4"];

    INIT  [label="init/\nmain.c", fillcolor="#FFE082"];
    KERN  [label="kernel/\nkernel_thread, syscalls", fillcolor="#FFCC80"];
    DRV   [label="drivers/\ndrv_*.c", fillcolor="#FFB74D"];
    HAL   [label="drivers/hal/\nstm32, infineon", fillcolor="#FFA726"];
    DRVAPP[label="drv_app/\napp-facing thin wrappers", fillcolor="#FFB74D"];
    SVC   [label="services/\n*_mgmt.c", fillcolor="#A1887F"];
    IPC   [label="ipc/", fillcolor="#90A4AE"];
    IRQ   [label="irq/", fillcolor="#90A4AE"];
    MM    [label="mm/", fillcolor="#90A4AE"];
    SAFE  [label="safety/\nwdog, safe_state", fillcolor="#EF9A9A"];
    LOG   [label="log/\nslog", fillcolor="#90A4AE"];
    LIB   [label="lib/\nCMSIS-DSP, FSM, BT", fillcolor="#B0BEC5"];
    SHELL [label="shell/", fillcolor="#80CBC4"];
    CONFIG[label="config/\nautoconf, Kconfig", fillcolor="#80DEEA"];
    ARCH  [label="arch/devices/<VENDOR>", fillcolor="#BCAAA4"];
    EX    [label="examples/<target>/", fillcolor="#CE93D8"];
    BUILD [label="build/<target>.elf", fillcolor="#F48FB1"];

    ROOT -> {INIT KERN DRV HAL DRVAPP SVC IPC IRQ MM SAFE LOG LIB SHELL CONFIG ARCH EX};
    {INIT KERN DRV HAL DRVAPP SVC IPC IRQ MM SAFE LOG LIB SHELL CONFIG ARCH EX} -> BUILD;
}
\enddot

---

## Boot Phase State Machine {#boot-phase-state-machine}

`init/main.c` implements an explicit FSM where every transition is
logged with `LOG_I("BOOT", ...)`.  Each phase is a value of the
`boot_phase_t` enum, stored in `_boot_phase` so a debugger can read
the current state on a crash.

\dot
digraph boot_fsm {
    rankdir=LR;
    node [shape=ellipse, fontname="Helvetica", fontsize=10, style=filled, fillcolor="#B3E5FC"];
    edge [fontname="Helvetica", fontsize=9];

    Reset [shape=doublecircle, fillcolor="#FFE0B2", label="Reset_Handler"];
    HAL   [label="BOOT_HAL_INIT\nHAL_Init()"];
    CLK   [label="BOOT_CLK_INIT\ndrv_rcc_clock_init()"];
    PERIPH[label="BOOT_PERIPH_CLK\nboard_clk_enable()"];
    IRQ   [label="BOOT_IRQ_INIT\ndrv_cpu_interrupt_prio_set\nirq_hw_init_all()"];
    IPC   [label="BOOT_IPC_INIT\nipc_mqueue_init()"];
    SLOG  [label="BOOT_SLOG_INIT\nslog_init()"];
    PREV  [label="BOOT_PREV_FAULT_CHK\nfault_handler / safe_state\nreplay & clear"];
    SRV   [label="BOOT_SERVICES_INIT\nos_kernel_thread_register()"];
    WDOG  [label="BOOT_WDOG_HW_INIT\nwdog_hw_init()\n(IRREVERSIBLE)", fillcolor="#FFCDD2"];
    APP   [label="BOOT_APP_INIT\napp_main()"];
    SCHED [label="BOOT_SCHEDULER\nvTaskStartScheduler()", fillcolor="#C8E6C9"];

    SAFE  [label="safety_enter_safe_state()", shape=octagon, fillcolor="#FFAB91"];

    Reset -> HAL -> CLK -> PERIPH -> IRQ -> IPC -> SLOG -> PREV -> SRV -> WDOG -> APP -> SCHED;
    HAL  -> SAFE [label="HAL_ERROR",        color="#D32F2F"];
    CLK  -> SAFE [label="!= OS_ERR_NONE",   color="#D32F2F"];
    IPC  -> SAFE [label="!= OS_ERR_NONE",   color="#D32F2F"];
    SRV  -> SAFE [label="register failed",  color="#D32F2F"];
    APP  -> SAFE [label="app_main != 0",    color="#D32F2F"];
    SCHED-> SAFE [label="returns\n(heap exhausted)", color="#D32F2F"];
}
\enddot

Source: [`init/main.c`](../init/main.c) — `_boot_enter()` advances the
FSM; `safety_enter_safe_state()` records the reason in `.noinit` and
halts.

Prose narrative: [`FLOW.md` § Phase 2 — main()](FLOW.md#phase-2--main).

---

## Boot Function-Call Chain {#boot-function-call-chain}

The full call tree from reset to the first task running.

\dot
digraph boot_chain {
    rankdir=TB;
    node [shape=box, fontname="Helvetica", fontsize=10, style="rounded,filled"];
    edge [fontname="Helvetica", fontsize=9];

    RESET [label="(hardware)\n0x08000000 → SP\n0x08000004 → Reset_Handler", fillcolor="#FFE0B2"];
    RH    [label="Reset_Handler\nstartup_stm32*.c", fillcolor="#FFD54F"];
    SI    [label="SystemInit\nhal_rcc_*.c (CMSIS)", fillcolor="#FFD54F"];
    DATA  [label=".data copy + .bss zero", fillcolor="#FFD54F"];
    LIBC  [label="__libc_init_array\n(C++ ctors)", fillcolor="#FFD54F"];

    MAIN  [label="main()\ninit/main.c", fillcolor="#81D4FA"];

    HAL    [label="HAL_Init()", fillcolor="#B3E5FC"];
    DRVRCC [label="drv_rcc_clock_init\n→ ops->clock_init\n→ hal_rcc_clock_init_stm32", fillcolor="#B3E5FC"];
    BCLK   [label="board_clk_enable\n(generated, per-board)", fillcolor="#B3E5FC"];
    PRIO   [label="drv_cpu_interrupt_prio_set\n→ NVIC_SetPriorityGrouping(3)", fillcolor="#B3E5FC"];
    IRQHW  [label="irq_hw_init_all\n(generated dispatch)", fillcolor="#B3E5FC"];
    MQ     [label="ipc_mqueue_init\n→ mqueue ring buffers", fillcolor="#B3E5FC"];
    SLOG   [label="slog_init\n→ ring buffer + level table", fillcolor="#B3E5FC"];

    REG    [label="os_kernel_thread_register", fillcolor="#A5D6A7"];

    WD  [label="wdog_service_start", fillcolor="#A5D6A7"];
    GP  [label="gpio_mgmt_start", fillcolor="#A5D6A7"];
    UM  [label="uart_mgmt_start", fillcolor="#A5D6A7"];
    IM  [label="iic_mgmt_start", fillcolor="#A5D6A7"];
    SM  [label="spi_mgmt_start", fillcolor="#A5D6A7"];
    TM  [label="task_mgr_start", fillcolor="#A5D6A7"];
    SH  [label="os_shell_mgmt_start", fillcolor="#A5D6A7"];

    WDHW  [label="wdog_hw_init\nIWDG → start", fillcolor="#FFAB91"];
    APPM  [label="app_main()\n(weak, override in examples/)", fillcolor="#FFCC80"];
    SCHED [label="vTaskStartScheduler()\nFreeRTOS kernel", fillcolor="#C8E6C9"];

    RESET -> RH -> SI -> DATA -> LIBC -> MAIN;
    MAIN -> HAL  -> DRVRCC -> BCLK -> PRIO -> IRQHW -> MQ -> SLOG -> REG -> WDHW -> APPM -> SCHED;
    REG -> WD; REG -> GP; REG -> UM; REG -> IM; REG -> SM; REG -> TM; REG -> SH;
}
\enddot

`os_kernel_thread_register()` (in `services/kernel_service_core.c`)
spawns every service thread via `os_thread_create()` — they are not
yet running because the scheduler is not started.  Once
`vTaskStartScheduler()` is called, the highest-priority ready task
runs first; service threads then take their staggered startup delay
(see the timing summary in [`DEV_MGMT.md`](DEV_MGMT.md#thread-timing-summary)).

---

## Thread Lifecycle {#thread-lifecycle}

Every task created by `os_thread_create()` follows the standard
FreeRTOS lifecycle.  The OS adds two project-specific touchpoints:
registration in the intrusive `os_thread_list_head` (for `task_mgr`
to enumerate) and software-watchdog check-in (for `wdog_service`).

\dot
digraph thread_lifecycle {
    rankdir=LR;
    node [shape=ellipse, style=filled, fontname="Helvetica", fontsize=10];
    edge [fontname="Helvetica", fontsize=9];

    NX  [label="non-existent",       fillcolor="#ECEFF1"];
    RDY [label="Ready",              fillcolor="#B3E5FC"];
    RUN [label="Running",            fillcolor="#A5D6A7"];
    BLK [label="Blocked\n(queue / notify / delay)", fillcolor="#FFE082"];
    SUS [label="Suspended\n(vTaskSuspend)", fillcolor="#CFD8DC"];
    DEL [label="Deleted",            fillcolor="#FFCDD2"];

    NX  -> RDY [label="os_thread_create"];
    RDY -> RUN [label="scheduler picks\n(priority order)"];
    RUN -> RDY [label="preempted /\nyield (tick)"];
    RUN -> BLK [label="xQueueReceive\nulTaskNotifyTake\nos_thread_delay"];
    BLK -> RDY [label="event /\ntimeout"];
    RUN -> SUS [label="vTaskSuspend"];
    SUS -> RDY [label="vTaskResume"];
    RUN -> DEL [label="vTaskDelete"];
}
\enddot

Source: see [`OS_THREAD.md` § Thread API](OS_THREAD.md#thread-api).

---

## Thread Map at Steady State {#thread-map-at-steady-state}

Every FreeRTOS task that exists in a typical build, with its priority,
stack size, and the queue / notification it consumes.  Arrow direction
shows control flow (a → b = "a posts to b's queue / notifies b").

\dot
digraph threads {
    rankdir=LR;
    node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=10];
    edge [fontname="Helvetica", fontsize=9];

    subgraph cluster_kernel {
        label="FreeRTOS Kernel Tasks";
        style="rounded,filled"; fillcolor="#E0F2F1";
        IDLE  [label="IDLE\nprio 0\n(kernel)",      fillcolor="#B2DFDB"];
        TIMR  [label="Tmr Svc\nprio configMAX-1\n(kernel)", fillcolor="#B2DFDB"];
    }

    subgraph cluster_safety {
        label="Safety";
        style="rounded,filled"; fillcolor="#FFEBEE";
        WDOG  [label="WDOG\nprio 4\nperiod 1 s\nfeeds IWDG", fillcolor="#FFCDD2"];
    }

    subgraph cluster_services {
        label="OS Service Threads";
        style="rounded,filled"; fillcolor="#E3F2FD";
        UART  [label="UART_MGMT\nprio 1\nqueue: _mgmt_queue", fillcolor="#BBDEFB"];
        IIC   [label="IIC_MGMT\nprio 1\nqueue: _mgmt_queue", fillcolor="#BBDEFB"];
        SPI   [label="SPI_MGMT\nprio 1\nqueue: _mgmt_queue", fillcolor="#BBDEFB"];
        GPIO  [label="GPIO_MGMT\nprio 1\nqueue: _mgmt_queue", fillcolor="#BBDEFB"];
        ADC   [label="ADC_MGMT\nprio 1\nper-channel queue", fillcolor="#BBDEFB"];
        SHELL [label="OS_SHELL\nprio 1\nblocks on UART RX", fillcolor="#BBDEFB"];
        MGR   [label="MGMT_TASK\nprio 1\nhealth + check-ins", fillcolor="#BBDEFB"];
    }

    subgraph cluster_app {
        label="Application Tasks (user)";
        style="rounded,filled"; fillcolor="#FFF8E1";
        APP1 [label="heartbeat\n(example)",       fillcolor="#FFE082"];
        APP2 [label="user task N\n…",             fillcolor="#FFE082"];
    }

    APP1 -> GPIO  [label="gpio_mgmt_post (toggle)"];
    APP1 -> UART  [label="printk → tx queue"];
    APP2 -> IIC   [label="iic_app_*"];
    APP2 -> SPI   [label="spi_app_*"];
    APP2 -> ADC   [label="adc_app_read"];

    SHELL -> UART [label="printf / rx"];
    MGR   -> WDOG [label="task_mgr_checkin"];
    WDOG  -> IDLE [label="IWDG reload (HW)", style=dashed];
}
\enddot

Detailed thread internals: [`DEV_MGMT.md`](DEV_MGMT.md).
Timing offsets: [`DEV_MGMT.md` § Thread Timing Summary](DEV_MGMT.md#thread-timing-summary).

---

## Service Thread Request / Notify Sequence {#service-thread-request--notify-sequence}

The canonical request/response pattern for a synchronous I²C read.
It is identical for SPI and UART (parameters differ).

\dot
digraph svc_seq {
    rankdir=TB;
    node [shape=box, fontname="Helvetica", fontsize=10, style="rounded,filled"];

    subgraph cluster_caller {
        label="Caller (application task)";
        style="rounded,filled"; fillcolor="#FFF8E1";
        APP1  [label="user task\niic_app_sync_read()"];
        APP2  [label="ulTaskNotifyTake (blocks)"];
    }

    subgraph cluster_mgmt {
        label="iic_mgmt_thread (prio 1)";
        style="rounded,filled"; fillcolor="#E3F2FD";
        Q1   [label="xQueueReceive\n(_mgmt_queue)"];
        DRV  [label="drv_iic_read()\n→ ops->read()"];
        HAL  [label="hal_iic_read_stm32\n→ HAL_I2C_Master_Receive_IT"];
        WAIT [label="ulTaskNotifyTake\non IRQ completion"];
        REPLY[label="xTaskNotifyGive(caller)"];
    }

    subgraph cluster_isr {
        label="I2C ISR (NVIC)";
        style="rounded,filled"; fillcolor="#FFEBEE";
        IRQ  [label="I2Cx_EV_IRQHandler\n→ flow_handler\n→ drv_irq_notify"];
        NOTE [label="vTaskNotifyGiveFromISR\n(target = mgmt task)"];
    }

    APP1 -> Q1   [label="xQueueSend(cmd)"];
    Q1   -> DRV  -> HAL -> WAIT;
    APP1 -> APP2 [label="block waiting"];

    IRQ  -> NOTE -> WAIT [label="wakes ISR-waiting task", style=dashed];
    WAIT -> REPLY -> APP2 [label="wakes caller", style=dashed];
}
\enddot

Each `*_mgmt_thread` serialises hardware access through one queue,
so concurrent application tasks never collide on the same bus.

---

## Driver vtable Architecture {#driver-vtable-architecture}

Layer 3 (`drv_uart` / `drv_iic` / `drv_spi` / `drv_gpio`) is purely
generic — it stores a pointer to an `ops` vtable that the HAL backend
populates at registration time.  Swapping vendor is a compile-time
choice of which `hal_*_<vendor>.c` to link.

\dot
digraph vtable {
    rankdir=LR;
    node [shape=record, fontname="Helvetica", fontsize=10, style=filled];

    Handle [label="{ drv_uart_handle | + ops : drv_uart_ops*  | + ctx : hal_uart_stm32_ctx*  | + tx_q, rx_q }", fillcolor="#BBDEFB"];

    Ops [label="{ drv_uart_ops (vtable) | + init() | + send() | + recv() | + set_baud() | + flush() }", fillcolor="#90CAF9"];

    HalSTM [label="{ hal_uart_stm32 | uart_init_stm32()  | uart_send_stm32()  | uart_recv_stm32()  | … }", fillcolor="#64B5F6"];

    HalINF [label="{ hal_uart_infineon | uart_init_inf()  | uart_send_inf()  | … }", fillcolor="#42A5F5"];

    HW [label="{ STM32 USART regs | RCC enable | IRQ enable | DMA req }", fillcolor="#BCAAA4"];

    Handle -> Ops [label=".ops"];
    Ops    -> HalSTM [label="bound at\nregister()", fontcolor="#1565C0"];
    Ops    -> HalINF [style=dashed, label="vendor swap"];
    HalSTM -> HW;
}
\enddot

The same pattern is used for every driver family.  See
[`DRIVERS.md` § Layer 3 — Generic Driver API](DRIVERS.md#layer-3--generic-driver-api).

---

## IRQ Flow — Hardware to Task {#irq-flow--hardware-to-task}

\dot
digraph irq {
    rankdir=LR;
    node [shape=box, fontname="Helvetica", fontsize=10, style="rounded,filled"];

    HW    [label="Peripheral\nfires interrupt",      fillcolor="#FFCDD2"];
    NVIC  [label="Cortex-M NVIC\nlooks up vector",   fillcolor="#FFCDD2"];
    VEC   [label="Vector table entry\n(weak default → strong @\nirq_periph_dispatch_generated.c)", fillcolor="#FFCDD2"];

    DISP  [label="<peripheral>_IRQHandler\n(generated dispatcher)", fillcolor="#FFE082"];
    FLOW  [label="<flow>_irq_handler\n(driver-level)",  fillcolor="#FFCC80"];

    DRV   [label="drv_irq_notify(irq_id)\nresolves desc + ops",fillcolor="#FFB74D"];
    NOTI  [label="vTaskNotifyGiveFromISR\nor xQueueSendFromISR", fillcolor="#FFAB91"];

    TASK  [label="*_mgmt_thread\nresumes from ulTaskNotifyTake",fillcolor="#A5D6A7"];
    DRVC  [label="driver completion\ncallback / queue post",fillcolor="#A5D6A7"];
    APP   [label="application\nresumes from blocking call", fillcolor="#FFE082"];

    HW -> NVIC -> VEC -> DISP -> FLOW -> DRV -> NOTI -> TASK -> DRVC -> APP;
}
\enddot

Cortex-M priority constraint: every IRQ that calls a FreeRTOS-from-ISR
API must use a numerical priority ≥ `configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY`
(= 5 on this project).  See [`IRQ.md`](IRQ.md) for the dispatch tables
and the priority-grouping rationale.

---

## Safety / Watchdog Loop {#safety--watchdog-loop}

\dot
digraph wdog {
    rankdir=LR;
    node [shape=box, fontname="Helvetica", fontsize=10, style="rounded,filled"];

    APP   [label="application task",        fillcolor="#FFE082"];
    REG   [label="wdog_task_register(self)",fillcolor="#FFCDD2"];
    CHK   [label="wdog_task_checkin()\n(every loop)", fillcolor="#FFCDD2"];

    WSVC  [label="wdog_service_thread\nprio 4, period 1 s",  fillcolor="#EF9A9A"];
    EVAL  [label="iterate registered tasks\ncheck last_checkin_age",fillcolor="#EF9A9A"];
    OK    [label="HAL_IWDG_Refresh()",      fillcolor="#A5D6A7"];
    BAD   [label="safety_enter_safe_state\nSAFE_REASON_TASK_STARVED",fillcolor="#E57373"];

    IWDG  [label="IWDG hardware\n(LSI, ~2 s timeout)\nresets MCU if not\nrefreshed", fillcolor="#BCAAA4", shape=box3d];

    APP -> REG -> CHK -> WSVC;
    WSVC -> EVAL;
    EVAL -> OK  [label="all tasks fresh"];
    EVAL -> BAD [label="any task starved"];
    OK   -> IWDG [label="HW refresh"];
    BAD  -> IWDG [label="(no refresh)\n→ HW reset", style=dashed];
}
\enddot

See [`ARCHITECTURE.md` § Medical-Grade Safety Layer](ARCHITECTURE.md#medical-grade-safety-layer)
for IEC 62304 traceability and the safe-state record format.

---

## Code-Gen Pipeline (XML → C) {#code-gen-pipeline-xml--c}

\dot
digraph codegen {
    rankdir=LR;
    node [shape=note, fontname="Helvetica", fontsize=10, style=filled];

    KC      [label="kconfig.conf\n(Kconfig snapshot)", fillcolor="#FFF59D"];
    BX      [label="board/<board>.xml", fillcolor="#FFCC80"];
    IX      [label="board/irq_table.xml", fillcolor="#FFCC80"];
    DX      [label="board/dsp_dev.xml", fillcolor="#FFCC80"];

    KCFE    [label="kconfig-frontends", shape=oval, fillcolor="#B3E5FC"];
    GBC     [label="gen_board_config.py", shape=oval, fillcolor="#B3E5FC"];
    GIRQ    [label="gen_irq_table.py", shape=oval, fillcolor="#B3E5FC"];
    GDSP    [label="arm_dsp_gen.py", shape=oval, fillcolor="#B3E5FC"];
    GHALC   [label="gen_stm32_hal_conf.py", shape=oval, fillcolor="#B3E5FC"];

    AH      [label="config/autoconf.h", fillcolor="#A5D6A7"];
    AM      [label="config/autoconf.mk", fillcolor="#A5D6A7"];
    HALCFG  [label="arch/.../stm32{f4,h7}xx_hal_conf.h", fillcolor="#A5D6A7"];

    BC      [label="board/board_config.c", fillcolor="#C8E6C9"];
    BH1     [label="board/board_device_ids.h", fillcolor="#C8E6C9"];
    BH2     [label="board/board_handles.h", fillcolor="#C8E6C9"];
    BH3     [label="board/mcu_config.h", fillcolor="#C8E6C9"];

    IRQHW   [label="board/irq_hw_init_generated.{c,h}", fillcolor="#C8E6C9"];
    IRQPD   [label="board/irq_periph_dispatch_generated.c", fillcolor="#C8E6C9"];
    IRQPH   [label="board/irq_periph_handlers_generated.h", fillcolor="#C8E6C9"];
    IRQTBL  [label="irq/irq_table.c", fillcolor="#C8E6C9"];

    DSPH    [label="config/dsp_config.h", fillcolor="#C8E6C9"];
    DSPM    [label="config/dsp_config.mk", fillcolor="#C8E6C9"];

    ELF     [label="build/<target>.elf", shape=box3d, fillcolor="#F48FB1"];

    KC -> KCFE -> AH; KCFE -> AM; KCFE -> HALCFG;
    BX -> GBC -> BC; GBC -> BH1; GBC -> BH2; GBC -> BH3;
    IX -> GIRQ -> IRQHW; GIRQ -> IRQPD; GIRQ -> IRQPH; GIRQ -> IRQTBL;
    DX -> GDSP -> DSPH; GDSP -> DSPM;
    AH  -> HALCFG -> GHALC;

    {AH AM HALCFG BC BH1 BH2 BH3 IRQHW IRQPD IRQPH IRQTBL DSPH DSPM} -> ELF;
}
\enddot

Prose: [`BOARD.md`](BOARD.md) (board XML schema, generator internals)
and [`IRQ.md` § IRQ Table Generator](IRQ.md#irq-table-generator).

---

## How to Add a New Diagram {#how-to-add-a-new-diagram}

1. Add a new `## Heading {#anchor}` section to this file.
2. Embed a Graphviz block between `\dot` and `\enddot` lines (no
   surrounding code fence).  Doxygen invokes `dot` and emits an SVG
   into `docs/doxygen/html/`.
3. Cross-link the new diagram from the relevant prose doc using
   a standard markdown link of the form `[link text]` immediately
   followed by `(DIAGRAMS.md#YOUR_SLUG)` — the slug must match the
   `{#...}` value chosen in step 1.
4. Re-run `make docs`.  No diagram source files (`.dot`, `.svg`) need
   to be committed — Doxygen regenerates everything from the markdown.

The Doxyfile already sets `DOT_IMAGE_FORMAT = svg` and
`INTERACTIVE_SVG = YES`, so the resulting graphs are zoomable in any
modern browser.
