# Static Analysis — CPPcheck & MISRA C:2012

This page documents the static analysis workflow for FreeRTOS-OS.  
All scripts live in [`scripts/`](../scripts/).

> **Doxygen users:** this page is part of the Doxygen-generated documentation. All report paths below are relative to `FreeRTOS-OS/`.

---

## Overview

| Tool | Role |
|---|---|
| **CPPcheck 2.x** | General C static analysis: null-pointer, memory leaks, UB, misuse of standard functions |
| **MISRA C:2012 addon** | Rule-compliance analysis via `misra.py` (bundled with CPPcheck) |

The workflow uses three scripts:

| Script | Purpose |
|---|---|
| [`scripts/install_cppcheck.sh`](../scripts/install_cppcheck.sh) | One-time installation of CPPcheck and all dependencies |
| [`scripts/run_cppcheck.sh`](../scripts/run_cppcheck.sh) | Runs analysis; produces text, XML, and optional HTML reports |
| [`scripts/cppcheck_suppressions.txt`](../scripts/cppcheck_suppressions.txt) | Project-level suppressions for documented deviations |

---

## Quick Start

```bash
# 1 — Install (once per machine)
./scripts/install_cppcheck.sh

# 2 — CPPcheck only  (errors + warnings)
./scripts/run_cppcheck.sh

# 3 — CPPcheck + MISRA C:2012
./scripts/run_cppcheck.sh --misra

# 4 — Full run: all severities, HTML report
./scripts/run_cppcheck.sh --misra --severity=style --html

# 5 — CI-fast mode: errors only, no HTML
./scripts/run_cppcheck.sh --severity=error
```

---

## Options Reference

| Option | Default | Description |
|---|---|---|
| `--misra` | off | Enable MISRA C:2012 checks via `misra.py` |
| `--misra-rules=<file>` | — | Path to MISRA rule-text file (unlocks descriptions) |
| `--severity=<level>` | `warning` | Minimum severity: `error` · `warning` · `style` · `performance` · `portability` · `information` |
| `--html` | off | Generate HTML report with `cppcheck-htmlreport` |
| `--output-dir=<dir>` | `reports/cppcheck` | Where to write all report files |
| `--jobs=<n>` | `nproc` | Parallel analysis jobs |
| `--verbose` | off | Print cppcheck invocation details |

---

## Reports

All reports are written to `reports/cppcheck/` (git-ignored).

| File | Format | Used by |
|---|---|---|
| `cppcheck_report.txt` | GCC-style text | Terminal / CI log |
| `cppcheck_report.xml` | CppCheck XML v2 | VS Code CPPcheck plugin, IDE integration |
| `misra_report.txt` | JSON-per-line | Custom tooling, review |
| `misra_cppcheck.xml` | CppCheck XML v2 (MISRA) | CI upload artefact |
| `html/index.html` | HTML | Browser review (`--html` required) |
| `misra_html/index.html` | HTML | MISRA browser report (`--html --misra` required) |

### Opening reports in VS Code

Install the **C/C++ Advanced Lint** or **CppCheck** extension and point it to  
`reports/cppcheck/cppcheck_report.xml`.

---

## MISRA C:2012 Rule-Text File

The MISRA C:2012 document is commercially licensed and not included in this repository.  
Without it, violations are shown as rule IDs (e.g., `misra-c2012-8.1`).  
If you own a copy, export the rules as plain text (one rule per line) and pass the path:

```bash
./scripts/run_cppcheck.sh --misra --misra-rules=/opt/misra_c_2012.txt
```

---

## Scope: What is Analysed

CPPcheck analyses **project-owned source files only**. Third-party code is excluded.

| Analysed | Excluded |
|---|---|
| `drivers/` | `arch/arm/CMSIS_6/` |
| `drv_app/`, `drv_ext_chips/` | `arch/devices/STM/` |
| `services/`, `shell/` | `kernel/FreeRTOS-Kernel/` |
| `ipc/`, `mm/`, `irq/`, `init/` | `kernel/FreeRTOS-Plus-CLI/` |
| `lib/`, `boot/` | `com_stack/canopen-stack/` (third-party) |
| `net/`, `com_stack/` (project files) | All `build/` directories |

---

## Preprocessor Configuration

The analysis uses the same defines that the compiler receives:

| Define | Value | Reason |
|---|---|---|
| `STM32F411xE` | 1 | Target MCU device selection |
| `USE_HAL_DRIVER` | 1 | Enables STM32 HAL header guards |
| `ARM_MATH_CM4` | 1 | CMSIS DSP library target |
| `__FPU_PRESENT` | 1 | Cortex-M4F FPU available |
| `__FPU_USED` | 1 | FPU enabled at runtime |
| `CORTEX_M4` | 1 | Architecture flag |
| `CONFIG_*` | per `autoconf.h` | All kernel Kconfig selections |

**Platform model:** `arm32-wchar_t4.xml` (bundled with CPPcheck) — 32-bit ARM, 4-byte `wchar_t`.

---

## Documented MISRA Deviations

The following MISRA C:2012 rules are suppressed project-wide with documented rationale. Any new deviation must be added here and to [`scripts/cppcheck_suppressions.txt`](../scripts/cppcheck_suppressions.txt).

| Rule | Category | Rationale |
|---|---|---|
| **Rule 15.5** | A function should have a single point of exit | FreeRTOS assert macros (`configASSERT`) expand to immediate return. Accepted as project deviation. |
| **Rule 21.6** | Standard I/O functions shall not be used | `printf`/ITM debug output is used only when `CONFIG_DEFAULT_DEBUG_EN=1`. Disabled in production builds. |
| **Rule 11.4** | Conversion between object pointer and integer | Required for MMIO register address mapping (`(volatile uint32_t *)0x40000000UL`). |
| **Rule 11.6** | Cast between `void *` and arithmetic type | FreeRTOS task parameter passing uses `void *`. No alternative within the FreeRTOS API contract. |
| **Rule 2.5** | No unused macro definitions | `CONFIG_*` macros from `autoconf.h` are conditionally consumed; not all appear in every translation unit. |
| **Rule 5.4** | Macro identifiers shall be distinct | STM32 HAL header guard naming collisions — not under project control. |
| **Rule 8.9** | Object at block scope if used in one function | Static ISR dispatch tables must be file-scope for symbol visibility across translation units. |

### Suppressing a false positive inline

For a single-line suppression in source code:

```c
// cppcheck-suppress misra-c2012-11.4
volatile uint32_t *reg = (volatile uint32_t *)PERIPH_BASE;
```

For a block:

```c
// cppcheck-suppress-begin misra-c2012-11.4
/* ... multiple lines ... */
// cppcheck-suppress-end misra-c2012-11.4
```

Always add a comment explaining why the suppression is valid.

---

## CI/CD Integration

The GitHub Actions workflow [`.github/workflows/static_analysis.yml`](../.github/workflows/static_analysis.yml) runs on every push and pull request:

| Job | Trigger | Fail condition |
|---|---|---|
| `cppcheck` | push / PR | Any `error`-severity finding |
| `misra` | push / PR | MISRA violations above threshold |

Reports are uploaded as GitHub Actions artefacts and retained for 30 days.

See [CI/CD pipeline](.github/workflows/static_analysis.yml) for the full workflow definition.

---

## Adding or Modifying Suppressions

1. Identify the false-positive rule ID from the report (e.g., `misra-c2012-11.4`).
2. Add an entry to [`scripts/cppcheck_suppressions.txt`](../scripts/cppcheck_suppressions.txt):
   ```
   misra-c2012-11.4:*/drivers/hal/stm32/hal_gpio_stm32.c
   ```
3. Add the deviation to the table above with rationale.
4. Commit both files together so the suppression is always paired with its documentation.

---

## Integrating with Doxygen

`docs/CHECK.md` (this file) is part of the Doxygen documentation. The `Doxyfile` includes `./docs` in its `INPUT` path, so the page is generated automatically with:

```bash
make docs        # or: doxygen Doxyfile
```

The generated HTML lands in `docs/` (index at `docs/index.html`).

---

*Maintained by: Subhajit Roy — subhajitroy005@gmail.com*
