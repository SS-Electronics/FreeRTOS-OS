# CMSIS-DSP Integration — FreeRTOS-OS

This document explains how ARM CMSIS-DSP function groups are selected, configured, code-generated, and compiled into the firmware. It covers the XML schema, the generator script, the Makefile integration, module dependency rules, and how to use the built-in PID controller in application code.

---

## Table of Contents

- [Concept](#concept)
- [Directory Layout](#directory-layout)
- [End-to-End Flow](#end-to-end-flow)
- [XML Schema — dsp_dev.xml](#xml-schema--dsp_devxml)
  - [Root Element](#root-element)
  - [Module Elements](#module-elements)
  - [Module Dependency Rules](#module-dependency-rules)
- [Code Generator — arm_dsp_gen.py](#code-generator--arm_dsp_genpy)
  - [Usage](#usage)
  - [Generated Files](#generated-files)
  - [dsp_config.h](#dsp_configh)
  - [dsp_config.mk](#dsp_configmk)
- [Makefile Integration](#makefile-integration)
  - [Include Paths Added](#include-paths-added)
  - [Source Files Compiled](#source-files-compiled)
  - [Controller Special Case](#controller-special-case)
- [Module Reference](#module-reference)
- [Flash Budget](#flash-budget)
- [Using the PID Controller](#using-the-pid-controller)
  - [Enabling the Module](#enabling-the-module)
  - [API](#api)
  - [Example — Accelerometer Tilt PID](#example--accelerometer-tilt-pid)
  - [Tuning Guide](#tuning-guide)
- [IntelliSense Configuration](#intellisense-configuration)

---

## Concept

CMSIS-DSP is a large library. Compiling the whole library into a Cortex-M4F firmware with 512 KB flash is not feasible. The DSP integration in FreeRTOS-OS provides:

1. **Selective compilation** — only the modules listed as `enabled="yes"` in `app/board/dsp_dev.xml` are compiled. All others produce zero code.
2. **Code-generated Makefile fragment** — `arm_dsp_gen.py` writes `dsp_config.mk` alongside the header so the build system reads the same source of truth as the C compiler.
3. **Guard macros** — `dsp_config.h` provides `CONFIG_ARM_DSP_*` defines. Application code wraps DSP calls in `#if CONFIG_ARM_DSP_CONTROLLER` (or the relevant module) to remain portable across builds where DSP is disabled.

---

## Directory Layout

```
FreeRTOS-OS-App/
│
├── app/
│   └── board/
│       ├── dsp_dev.xml              ← user-authored DSP module selection (edit this)
│       ├── dsp_config.h             ← generated — do not edit
│       └── dsp_config.mk            ← generated — do not edit
│
└── FreeRTOS-OS/
    ├── scripts/
    │   └── arm_dsp_gen.py           ← generator: XML → dsp_config.h + dsp_config.mk
    ├── lib/
    │   ├── Makefile                 ← includes dsp_config.mk, adds DSP obj-y entries
    │   └── CMSIS-DSP/
    │       ├── Include/             ← arm_math.h, dsp/controller_functions.h, …
    │       ├── PrivateInclude/      ← internal CMSIS-DSP headers
    │       └── Source/
    │           ├── BasicMathFunctions/
    │           ├── ControllerFunctions/
    │           ├── FilteringFunctions/
    │           └── …
    └── arch/
        └── arm/
            └── CMSIS_6/
                └── CMSIS/Core/Include/   ← cmsis_compiler.h (required by arm_math_types.h)
```

`dsp_dev.xml` and the two generated files are application-owned and live alongside the other board configuration artefacts in `app/board/`. Do not hand-edit the generated files; re-run the generator after any XML change.

---

## End-to-End Flow

```
app/board/dsp_dev.xml
        │
        ▼  python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml
        │
        ├──▶  app/board/dsp_config.h        (C header — #include in firmware)
        └──▶  app/board/dsp_config.mk       (Makefile fragment — -include in lib/Makefile)

make app
        │
        ├── lib/Makefile  -include $(APP_DIR)/board/dsp_config.mk
        │       │
        │       └── ifeq ($(CONFIG_ARM_DSP_CONTROLLER),1)
        │               obj-y += lib/CMSIS-DSP/Source/ControllerFunctions/arm_pid_init_f32.o
        │               endif
        │
        └── app/app_main.c
                #include <board/dsp_config.h>
                #if CONFIG_ARM_DSP_CONTROLLER
                #include <arm_math.h>
                arm_pid_instance_f32 pid;
                arm_pid_init_f32(&pid, 1);
                float32_t out = arm_pid_f32(&pid, error);
                #endif
```

---

## XML Schema — dsp_dev.xml

### Root Element

```xml
<dsp_config target="STM32F411xE"
            fpu="cortex-m4"
            description="…">
  <modules> … </modules>
</dsp_config>
```

| Attribute     | Required | Description |
|---|---|---|
| `target`      | yes | MCU part number (informational, not parsed by the generator) |
| `fpu`         | yes | FPU type (informational; `cortex-m4` for Cortex-M4F) |
| `description` | no  | Human-readable label written into the generated file banner |

### Module Elements

```xml
<module name="basic_math" enabled="yes"/>
```

| Attribute | Values | Description |
|---|---|---|
| `name`    | see [Module Reference](#module-reference) | DSP module group name |
| `enabled` | `yes` \| `no` | Whether to compile and expose this module |

Unknown `name` values are reported as warnings by the generator and ignored.

### Module Dependency Rules

Some modules have hard link-time dependencies on other modules:

| Module | Depends on | Reason |
|---|---|---|
| `controller` | `common_tables` | `arm_sin_cos_f32/q31` need `sinTable_f32/q31` |
| `transform` | `common_tables` + `matrix` | `arm_cfft_*` needs twiddle tables; `arm_mfcc_*` needs `arm_mat_vec_mult_*` |
| `fast_math` | `common_tables` | `arm_sin_f32/cos_f32` use the same sin table |

**Exception — PID only:** if you enable `controller` solely for PID (`arm_pid_f32` / `arm_pid_init_f32`) and do not need `arm_sin_cos_f32` (Clarke/Park), `common_tables` is **not** required. The Makefile compiles only `arm_pid_init_f32.c` for the controller module rather than the aggregate `ControllerFunctions.c`, avoiding the sin/cos dependency. See [Controller Special Case](#controller-special-case).

---

## Code Generator — arm_dsp_gen.py

### Usage

```bash
# From the project root:
python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml

# Explicit output directory (default: same directory as the XML file):
python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml --outdir app/board
```

Re-run the generator whenever `dsp_dev.xml` changes, then rebuild:

```bash
python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml && make app
```

### Generated Files

| File | Purpose |
|---|---|
| `app/board/dsp_config.h` | C header included by application code to gate DSP calls |
| `app/board/dsp_config.mk` | Makefile fragment included by `lib/Makefile` to select compiled modules |

Both files carry a `AUTO-GENERATED — DO NOT EDIT` banner and a re-generate recipe.

### dsp_config.h

```c
#ifndef BOARD_DSP_CONFIG_H_
#define BOARD_DSP_CONFIG_H_

/* Master enable — 1 when at least one module is active */
#define CONFIG_ARM_DSP_ENABLED          1

/* Per-module enables */
#define CONFIG_ARM_DSP_COMMON_TABLES    0
#define CONFIG_ARM_DSP_BASIC_MATH       0
#define CONFIG_ARM_DSP_CONTROLLER       1  /* PID controller, Clarke/Park */
#define CONFIG_ARM_DSP_FAST_MATH        0
/* … */

#endif
```

Include this header before `arm_math.h` to drive conditional compilation:

```c
#include <board/dsp_config.h>
#if CONFIG_ARM_DSP_CONTROLLER
#include <arm_math.h>
#endif
```

### dsp_config.mk

```makefile
CONFIG_ARM_DSP_ENABLED          := 1
CONFIG_ARM_DSP_COMMON_TABLES    := 0
CONFIG_ARM_DSP_CONTROLLER       := 1
# …
```

Consumed by `lib/Makefile` via `-include $(APP_DIR)/board/dsp_config.mk`. The leading dash makes the include silent when `APP_DIR` is not set (standalone kernel build).

---

## Makefile Integration

`FreeRTOS-OS/lib/Makefile` contains the full DSP block. Key excerpts:

```makefile
-include $(APP_DIR)/board/dsp_config.mk

DSP_ROOT := $(CURDIR)/lib/CMSIS-DSP

ifeq ($(CONFIG_ARM_DSP_ENABLED),1)

INCLUDES += -I$(DSP_ROOT)/Include
INCLUDES += -I$(DSP_ROOT)/PrivateInclude
INCLUDES += -I$(CURDIR)/arch/arm/CMSIS_6/CMSIS/Core/Include

ifeq ($(CONFIG_ARM_DSP_CONTROLLER),1)
obj-y += lib/CMSIS-DSP/Source/ControllerFunctions/arm_pid_init_f32.o
endif

# … per-module obj-y entries …

endif # CONFIG_ARM_DSP_ENABLED
```

### Include Paths Added

When `CONFIG_ARM_DSP_ENABLED=1` the following paths are added to the compiler search path:

| Path | Provides |
|---|---|
| `lib/CMSIS-DSP/Include` | `arm_math.h`, `dsp/controller_functions.h`, … |
| `lib/CMSIS-DSP/PrivateInclude` | Internal CMSIS-DSP headers (arm_vec_fft.h, arm_sorting.h, …) |
| `arch/arm/CMSIS_6/CMSIS/Core/Include` | `cmsis_compiler.h`, `cmsis_gcc.h` (required by `arm_math_types.h`) |

### Source Files Compiled

| Makefile variable | Source compiled |
|---|---|
| `CONFIG_ARM_DSP_COMMON_TABLES=1` | `Source/CommonTables/CommonTables.c` |
| `CONFIG_ARM_DSP_BASIC_MATH=1` | `Source/BasicMathFunctions/BasicMathFunctions.c` |
| `CONFIG_ARM_DSP_COMPLEX_MATH=1` | `Source/ComplexMathFunctions/ComplexMathFunctions.c` |
| `CONFIG_ARM_DSP_CONTROLLER=1` | `Source/ControllerFunctions/arm_pid_init_f32.c` *(see below)* |
| `CONFIG_ARM_DSP_FAST_MATH=1` | `Source/FastMathFunctions/FastMathFunctions.c` |
| `CONFIG_ARM_DSP_FILTERING=1` | `Source/FilteringFunctions/FilteringFunctions.c` |
| `CONFIG_ARM_DSP_INTERPOLATION=1` | `Source/InterpolationFunctions/InterpolationFunctions.c` |
| `CONFIG_ARM_DSP_MATRIX=1` | `Source/MatrixFunctions/MatrixFunctions.c` |
| `CONFIG_ARM_DSP_STATISTICS=1` | `Source/StatisticsFunctions/StatisticsFunctions.c` |
| `CONFIG_ARM_DSP_SUPPORT=1` | `Source/SupportFunctions/SupportFunctions.c` |
| `CONFIG_ARM_DSP_TRANSFORM=1` | `Source/TransformFunctions/TransformFunctions.c` |
| `CONFIG_ARM_DSP_BAYES=1` | `Source/BayesFunctions/BayesFunctions.c` |
| `CONFIG_ARM_DSP_SVM=1` | `Source/SVMFunctions/SVMFunctions.c` |
| `CONFIG_ARM_DSP_DISTANCE=1` | `Source/DistanceFunctions/DistanceFunctions.c` |
| `CONFIG_ARM_DSP_QUATERNION=1` | `Source/QuaternionMathFunctions/QuaternionMathFunctions.c` |
| `CONFIG_ARM_DSP_WINDOW=1` | `Source/WindowFunctions/WindowFunctions.c` |

Most modules use the aggregate `.c` file that `#include`s every individual `.c` in the module directory.

### Controller Special Case

`ControllerFunctions.c` is **not** used. It includes `arm_sin_cos_f32.c` and `arm_sin_cos_q31.c`, which reference `sinTable_f32` and `sinTable_q31` defined in `CommonTables`. Those tables are part of a 45 000-line file that also contains all FFT twiddle factors — several hundred KB of rodata.

For pure PID use, only `arm_pid_init_f32.c` is compiled. `arm_pid_f32` is declared `__STATIC_FORCEINLINE` in `dsp/controller_functions.h` and requires no compiled object. This avoids pulling in `CommonTables` entirely.

To also use `arm_sin_cos_f32` (required for Clarke/Park transforms), enable `common_tables` in `dsp_dev.xml` and replace `arm_pid_init_f32.o` with `ControllerFunctions.o` in `lib/Makefile`.

---

## Module Reference

| XML name | Macro suffix | Header | Typical use |
|---|---|---|---|
| `common_tables` | `COMMON_TABLES` | — | FFT twiddle factors, sin/cos tables; dependency of transform/controller |
| `basic_math` | `BASIC_MATH` | `dsp/basic_math_functions.h` | add, sub, mult, abs, dot-product, scale, shift, clip |
| `complex_math` | `COMPLEX_MATH` | `dsp/complex_math_functions.h` | complex multiply, magnitude, dot-product, conjugate |
| `controller` | `CONTROLLER` | `dsp/controller_functions.h` | PID controller, Clarke/Park transforms |
| `fast_math` | `FAST_MATH` | `dsp/fast_math_functions.h` | `arm_sin_f32`, `arm_cos_f32`, `arm_sqrt_f32` |
| `filtering` | `FILTERING` | `dsp/filtering_functions.h` | FIR, IIR/biquad, LMS, convolution, correlation, decimation |
| `interpolation` | `INTERPOLATION` | `dsp/interpolation_functions.h` | linear, bilinear, spline |
| `matrix` | `MATRIX` | `dsp/matrix_functions.h` | add, sub, mult, inverse, transpose, Cholesky |
| `statistics` | `STATISTICS` | `dsp/statistics_functions.h` | mean, power, RMS, variance, min, max, entropy |
| `support` | `SUPPORT` | `dsp/support_functions.h` | copy, fill, f32/q31/q15/q7 type conversion |
| `transform` | `TRANSFORM` | `dsp/transform_functions.h` | CFFT, RFFT, DCT4 (requires `common_tables` + `matrix`) |
| `bayes` | `BAYES` | `dsp/bayes_functions.h` | Naive Bayes classifier (float32) |
| `svm` | `SVM` | `dsp/svm_functions.h` | SVM: linear, polynomial, RBF, sigmoid |
| `distance` | `DISTANCE` | `dsp/distance_functions.h` | Minkowski, Canberra, Chebyshev, cityblock, … |
| `quaternion` | `QUATERNION` | `dsp/quaternion_math_functions.h` | multiply, norm, inverse, conjugate, log, exp |
| `window` | `WINDOW` | `dsp/window_functions.h` | Bartlett, Hann, Hamming, Blackman, Nuttall, … |

---

## Flash Budget

Approximate `.text` + `.rodata` cost per module compiled for Cortex-M4F at `-O2`:

| Module | Approx. flash |
|---|---|
| `arm_pid_init_f32.c` only | < 1 KB |
| `basic_math` | ~15 KB |
| `fast_math` | ~5 KB |
| `filtering` | ~35 KB |
| `statistics` | ~10 KB |
| `support` | ~8 KB |
| `transform` | ~25 KB |
| `common_tables` (full) | ~400 KB — includes all FFT twiddle tables |

STM32F411xE has 512 KB flash. The baseline OS + app firmware is ~180 KB, leaving ~330 KB for DSP. Enable only the modules the application actively calls.

---

## Using the PID Controller

### Enabling the Module

1. Set `enabled="yes"` for `controller` in `app/board/dsp_dev.xml`.
2. Regenerate:
   ```bash
   python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml
   ```
3. Rebuild:
   ```bash
   make app
   ```

`common_tables` does **not** need to be enabled for PID-only use (see [Controller Special Case](#controller-special-case)).

### API

```c
#include <board/dsp_config.h>
#if CONFIG_ARM_DSP_CONTROLLER
#include <arm_math.h>

/* 1. Declare and configure the instance */
arm_pid_instance_f32 pid;
pid.Kp = 1.0f;
pid.Ki = 0.05f;
pid.Kd = 0.1f;

/* 2. Initialise (resetStateFlag=1 clears the state array to zero) */
arm_pid_init_f32(&pid, 1);

/* 3. Run one PID step — call once per sample period */
/*    in  = error = setpoint − process_variable        */
float32_t error  = setpoint - process_variable;
float32_t output = arm_pid_f32(&pid, error);
#endif
```

The discrete-time equation implemented by `arm_pid_f32`:

```
y[n] = y[n-1]  +  A0·e[n]  +  A1·e[n-1]  +  A2·e[n-2]

where  A0 = Kp + Ki + Kd
       A1 = −Kp − 2·Kd
       A2 = Kd
```

This is a positional-form PID. The output accumulates over time; clamp it to the actuator range after each call.

### Example — Accelerometer Tilt PID

`app_main.c` demonstrates the PID running inside `acc_poll_task` at 1 Hz, using the LSM303DLHC X-axis acceleration as the process variable:

```c
/* Configuration — top of app_main.c */
#define PID_KP            0.10f
#define PID_KI            0.02f
#define PID_KD            0.05f
#define PID_SETPOINT_MG   0.0f   /* target: 0 mg on X-axis (board level) */
#define PID_OUTPUT_MAX    100.0f
#define PID_OUTPUT_MIN   -100.0f

volatile float32_t g_pid_output = 0.0f;  /* last computed output */
```

```c
/* Inside acc_poll_task() — runs every APP_ACC_POLL_PERIOD ms */
arm_pid_instance_f32 pid;
pid.Kp = PID_KP;  pid.Ki = PID_KI;  pid.Kd = PID_KD;
arm_pid_init_f32(&pid, 1);

for (;;) {
    lsm303dlhc_acc_read(I2C_SENSOR_BUS, &raw);
    int mg_x = (int)(raw.x / 16);

    float32_t error  = PID_SETPOINT_MG - (float32_t)mg_x;
    float32_t output = arm_pid_f32(&pid, error);

    if (output > PID_OUTPUT_MAX) output = PID_OUTPUT_MAX;
    if (output < PID_OUTPUT_MIN) output = PID_OUTPUT_MIN;

    g_pid_output = output;
    printk("[PID] sp=%.0f  pv=%d  err=%.0f  out=%.2f\r\n",
           (double)PID_SETPOINT_MG, mg_x,
           (double)error, (double)output);

    os_thread_delay(APP_ACC_POLL_PERIOD);
}
```

Replace `PID_SETPOINT_MG` with the desired physical setpoint (temperature, speed, angle, etc.) and map `output` to the actual actuator (PWM duty cycle, DAC value, motor command).

### Tuning Guide

| Step | Action |
|---|---|
| Start | Set `Ki = 0`, `Kd = 0`. Increase `Kp` until the output oscillates. |
| Proportional | Reduce `Kp` to ~60 % of the oscillation value. |
| Integral | Increase `Ki` slowly until steady-state error is eliminated. |
| Derivative | Add a small `Kd` (typically 0.1 × Kp) to reduce overshoot. |
| Sample rate | Call `arm_pid_f32` at a **fixed** period. A 10 × faster sample rate requires Ki/10 and Kd×10 to maintain the same real-time gains. |

For a 1 Hz sample rate, the defaults in `app_main.c` (`Kp = 0.10`, `Ki = 0.02`, `Kd = 0.05`) give gentle, stable response suitable for slow physical systems.

---

## IntelliSense Configuration

`.vscode/c_cpp_properties.json` must include the CMSIS-DSP paths and `CONFIG_ARM_DSP_*` defines so the IDE resolves `arm_math.h` and `float32_t` correctly. The following entries are required:

**`includePath`:**
```json
"${workspaceFolder}/FreeRTOS-OS/arch/arm/CMSIS_6/CMSIS/Core/Include",
"${workspaceFolder}/FreeRTOS-OS/lib/CMSIS-DSP/Include",
"${workspaceFolder}/FreeRTOS-OS/lib/CMSIS-DSP/PrivateInclude"
```

**`defines`:**
```json
"CONFIG_ARM_DSP_ENABLED=1",
"CONFIG_ARM_DSP_CONTROLLER=1"
```

These entries are already present in `.vscode/c_cpp_properties.json`. If you enable additional modules, add their `CONFIG_ARM_DSP_*=1` defines to the same file.
