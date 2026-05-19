#!/usr/bin/env python3
"""
apply_doxygen_header.py — normalise the file-level Doxygen header of
every first-party FreeRTOS-OS source file.

The script walks a hand-picked allow-list of directories (skipping
vendored submodules and generated outputs), then for every .c/.h
file:

  1. Detects the existing leading C-block comment, if any.
  2. Extracts a one-line @brief from that comment (best effort).
  3. Writes the standard FreeRTOS-OS Doxygen header at the top, with
     @file / @brief / @ingroup / @author / @module / @info /
     @dependency / @copyright fields filled in.
  4. Preserves all code below the header verbatim.

Run from the repo root:
    python3 scripts/apply_doxygen_header.py            # apply
    python3 scripts/apply_doxygen_header.py --check    # diff-only

The mapping between directory and Doxygen group key lives in
DIR_TO_GROUP below.  Add an entry whenever a new top-level dir is
added that should appear in the generated docs tree.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# ───────────────────────────────────────────────────────────────────────────
# Directory → Doxygen @ingroup key
# Order matters — the first matching prefix wins.
# ───────────────────────────────────────────────────────────────────────────
DIR_TO_GROUP = [
    ("init/",                       "boot"),
    ("mm/",                         "mm"),
    ("ipc/",                        "ipc"),
    ("irq/",                        "irq"),
    ("log/",                        "log"),
    ("safety/",                     "safety"),
    ("shell/",                      "shell"),
    ("services/",                   "services"),
    ("drv_app/",                    "drv_app"),
    ("drv_ext_chips/",              "drv_ext_chips"),
    ("drivers/hal/stm32/",          "hal_stm32"),
    ("drivers/hal/infineon/",       "hal_infineon"),
    ("drivers/",                    "drivers"),
    ("kernel/kernel_thread.c",      "kernel_glue"),
    ("kernel/syscalls.c",           "kernel_glue"),
    ("kernel/sysmem.c",             "kernel_glue"),
    ("kernel/system.c",             "kernel_glue"),
    ("include/drivers/hal/",        "drivers"),
    ("include/drivers/",            "drivers"),
    ("include/drv_app/",            "drv_app"),
    ("include/drv_ext_chips/",      "drv_ext_chips"),
    ("include/init/",               "boot"),
    ("include/ipc/",                "ipc"),
    ("include/irq/",                "irq"),
    ("include/log/",                "log"),
    ("include/mm/",                 "mm"),
    ("include/safety/",             "safety"),
    ("include/services/",           "services"),
    ("include/shell/",              "shell"),
    ("include/os/",                 "public_api"),
    ("include/def_",                "config"),
    ("include/",                    "public_api"),
]

# ───────────────────────────────────────────────────────────────────────────
# Files that must be processed.  Globs are evaluated relative to repo root.
# ───────────────────────────────────────────────────────────────────────────
INCLUDE_GLOBS = [
    "init/*.c", "init/*.h",
    "mm/*.c",
    "ipc/*.c",
    "irq/*.c",
    "log/*.c",
    "safety/*.c",
    "shell/*.c",
    "services/*.c",
    "drv_app/*.c",
    "drv_ext_chips/*.c",
    "drivers/*.c",
    "drivers/hal/stm32/*.c",
    "drivers/hal/infineon/*.c",
    "kernel/kernel_thread.c",
    "kernel/syscalls.c",
    "kernel/sysmem.c",
    "kernel/system.c",
    "include/*.h",
    "include/drivers/*.h",
    "include/drivers/hal/stm32/*.h",
    "include/drv_app/*.h",
    "include/drv_ext_chips/*.h",
    "include/drv_ext_chips/**/*.h",
    "include/init/*.h",
    "include/ipc/*.h",
    "include/irq/*.h",
    "include/log/*.h",
    "include/mm/*.h",
    "include/os/*.h",
    "include/safety/*.h",
    "include/services/*.h",
    "include/shell/*.h",
]

# Files to skip even if they match the globs above.
EXCLUDE_NAMES = {
    "doxygen_groups.h",   # already authored manually
    "autoconf.h",         # generated
}

# Files in include/drv_ext_chips/<chip>/ that are .c — these are real
# implementations placed unconventionally under include/.  Add them as
# well, since they ship in the build.
INCLUDE_GLOBS.append("include/drv_ext_chips/**/*.c")

# ───────────────────────────────────────────────────────────────────────────
# Header templates
# ───────────────────────────────────────────────────────────────────────────
TEMPLATE_HEAD = """\
/**
 * @file        {filename}
 * @brief       {brief}
 * @ingroup     {group}
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      {module}
 * @info        {info}
 * @dependency  {dependency}
 *
"""

TEMPLATE_DETAILS = """\
 * @details
{details_body}
 *
"""

TEMPLATE_TAIL = """\
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */
"""

# Per-directory module / info / dependency strings.  Falls back to
# generic strings derived from the group key.
DIR_META = {
    "boot":          ("Boot & Init",     "System bring-up: HAL init, clock tree, IRQ tables, services, scheduler entry.",                "FreeRTOS, HAL, IRQ table, board BSP"),
    "mm":            ("Memory Mgmt",     "Heap helpers, intrusive doubly-linked list (Linux-style), DMA pool allocator.",                  "FreeRTOS heap_4, list.h"),
    "ipc":           ("IPC",             "Inter-process communication: ring buffers (HW byte streams) and xQueue-based message queues.",   "FreeRTOS queue, ringbuffer.h"),
    "irq":           ("IRQ Subsystem",   "Linux-style irq_desc chain, software IRQ table, request_irq/free_irq, NVIC chip driver.",        "Generated irq_table.c, NVIC chip"),
    "log":           ("Logging",         "32-entry ring-buffer structured logger with severity levels; safe pre-scheduler & from ISRs.",  "Compiler intrinsics, optional printk echo"),
    "safety":        ("Safety",          "Medical-grade safety stack: IWDG, per-task software watchdog, safe-state shutdown.",             "STM32 HAL IWDG, .noinit RAM section"),
    "shell":         ("Shell",           "Interactive shell over UART_APP using FreeRTOS-Plus-CLI; registers OS introspection commands.",  "FreeRTOS-Plus-CLI, UART mgmt service"),
    "services":      ("Services",        "FreeRTOS service threads that own peripherals (UART/I2C/SPI/GPIO/ADC) and accept commands.",     "Driver layer, ipc/mqueue, board config"),
    "drv_app":       ("Driver App",      "Application-facing helpers that wrap driver init for board-listed peripherals.",                 "Driver layer, board BSP"),
    "drv_ext_chips": ("External Chips",  "Drivers for off-MCU peripherals (sensors, GPIO expanders, EEPROM, DACs).",                       "Driver layer (I2C / SPI)"),
    "drivers":       ("Driver Layer",    "Vendor-agnostic driver vtables; concrete backends live under drivers/hal/<vendor>/.",            "HAL backend (selected by CONFIG_DEVICE_VARIANT)"),
    "hal_stm32":     ("STM32 HAL",       "STM32-specific HAL backend implementing the generic driver vtables for STM32F4 / STM32H7.",      "stm32f4xx-hal-driver, stm32h7xx-hal-driver"),
    "hal_infineon":  ("Infineon HAL",    "Infineon XMC HAL backend stubs implementing the generic driver vtables (work-in-progress).",     "Infineon XMCLib (placeholder)"),
    "config":        ("Definitions",     "Project-wide attribute / compiler / standard-type macros and error code enums.",                 "Compiler intrinsics, CMSIS"),
    "public_api":    ("Public API",      "Public API surface — included by application code and out-of-tree drivers.",                     "FreeRTOS, def_std.h"),
    "kernel_glue":   ("Kernel Glue",     "C-runtime / newlib shim: malloc/free wrappers, syscalls, FreeRTOS kernel adapters.",             "newlib-nano, FreeRTOS-Kernel"),
}


# ───────────────────────────────────────────────────────────────────────────
# Helpers
# ───────────────────────────────────────────────────────────────────────────

LEADING_COMMENT_RE = re.compile(
    r"\A\s*/\*[\s\S]*?\*/\s*", re.MULTILINE
)

# Capture text on the first @brief / brief: / @file <name> — <brief> line.
BRIEF_AT_RE     = re.compile(r"@brief\s+(.+)")
BRIEF_FILE_RE   = re.compile(r"@file\s+\S+\s*[-–—]\s*(.+)")
FIRST_LINE_RE   = re.compile(r"^\s*\*?\s*(\S[^\n]*?)\s*$", re.MULTILINE)


def derive_group(rel_path: str) -> str:
    for prefix, key in DIR_TO_GROUP:
        if rel_path.startswith(prefix):
            return key
    return "os"


def derive_brief_from_existing(comment: str, default: str) -> str:
    if not comment:
        return default
    m = BRIEF_AT_RE.search(comment)
    if m:
        return m.group(1).strip().rstrip(".")
    m = BRIEF_FILE_RE.search(comment)
    if m:
        return m.group(1).strip().rstrip(".")
    # Fall back to the first non-empty content line of the comment that is
    # NOT GPL / authorship boilerplate.
    for line in comment.splitlines():
        # Drop the gutter "  *  " or "  # "
        stripped = line.strip()
        stripped = stripped.lstrip("/*").rstrip("*/").strip()
        stripped = stripped.lstrip("#*").strip()
        if not stripped:
            continue
        if any(pat.match(line) for pat in DROP_PATTERNS):
            continue
        if stripped.startswith("@"):
            continue
        if stripped.lower().startswith(("file:", "module", "moudle", "info:",
                                        "dependency:", "author:", "copyright")):
            continue
        if "is part of" in stripped.lower():
            continue
        return stripped.rstrip(".")
    return default


# Tags that mark structured Doxygen content worth preserving.
PRESERVE_TAGS = (
    "@details", "@note", "@warning", "@code", "@endcode",
    "@param", "@return", "@retval", "@see", "@usage",
    "@example", "@startuml", "@enduml",
)

# Existing header lines that we deliberately drop because we re-emit them
# in the new standard form.
# The gutter is matched as "  * " OR "  # " (Python-comment-style block).
_G = r"^\s*[\*#]?\s*"
DROP_PATTERNS = (
    re.compile(_G + r"@file\b.*$"),
    re.compile(_G + r"@brief\b.*$"),
    re.compile(_G + r"@ingroup\b.*$"),
    re.compile(_G + r"@details\b\s*$"),                # bare "@details" line
    re.compile(_G + r"@author\b.*$", re.IGNORECASE),
    re.compile(_G + r"@module\b.*$", re.IGNORECASE),
    re.compile(_G + r"@info\b.*$", re.IGNORECASE),
    re.compile(_G + r"@dependency\b.*$", re.IGNORECASE),
    re.compile(_G + r"@copyright\b.*$"),
    re.compile(_G + r"@date\b.*$"),
    re.compile(_G + r"File:.*$"),
    re.compile(_G + r"Author:.*$"),
    re.compile(_G + r"Module:.*$", re.IGNORECASE),
    re.compile(_G + r"Moudle:.*$", re.IGNORECASE),
    re.compile(_G + r"Info:.*$", re.IGNORECASE),
    re.compile(_G + r"Dependency:.*$", re.IGNORECASE),
    re.compile(_G + r"subhajitroy005@gmail\.com.*$"),
    # GPL boilerplate — emitted by template tail
    re.compile(_G + r"Copyright\s*\(C\).*$", re.IGNORECASE),
    re.compile(_G + r"Copyright\s+(20|19)\d{2}\b.*$", re.IGNORECASE),
    re.compile(_G + r"This file is part of\b.*$", re.IGNORECASE),
    re.compile(_G + r"(FreeRTOS|RTOS).{0,40}is free software.*$"),
    re.compile(_G + r"(modify\s+)?it under the terms.*$"),
    re.compile(_G + r"as published by(\s+the Free)?.*$"),
    re.compile(_G + r"\d+ of the License.*$"),
    re.compile(_G + r"the Free Software Foundation.*$"),
    re.compile(_G + r"\(at your option\) any later version.*$"),
    re.compile(_G + r"(FreeRTOS|RTOS).{0,40}is distributed.*$"),
    re.compile(_G + r"but WITHOUT ANY WARRANTY.*$"),
    re.compile(_G + r"MERCHANTABILITY or FITNESS.*$"),
    re.compile(_G + r"(of\s+the\s+)?GNU General Public License.*$"),
    re.compile(_G + r"You should have received.*$"),
    re.compile(_G + r"License along with.*$"),
    re.compile(_G + r"along with (FreeRTOS|RTOS|FreeRTOS-KERNEL).*$"),
    re.compile(_G + r"<https?://www\.gnu\.org/licenses.*$"),
    re.compile(_G + r"If not, see.*$"),
    re.compile(_G + r"without even the implied warranty.*$", re.IGNORECASE),
    re.compile(_G + r"you can redistribute.*$"),
    re.compile(_G + r"redistribute it and/or modify.*$"),
    re.compile(_G + r"version\s+\d+\s+of the License.*$"),
    re.compile(_G + r"Foundation,\s*either version.*$"),
    re.compile(_G + r"in the hope that it will be useful.*$"),
    re.compile(_G + r"(any|later)\s+version.*$"),
    # SPDX tags
    re.compile(_G + r"SPDX-License-Identifier:.*$"),
)


def extract_details_body(comment: str) -> str:
    """Pull preservation-worthy lines out of the existing leading comment.

    Returns the inner body for @details *without* the surrounding /** */
    wrappers, formatted as ' *  <text>' lines.  Empty result means there
    was no rich content worth keeping.
    """
    if not comment:
        return ""

    # Strip the opening /* ... and trailing */.
    inner = re.sub(r"\A\s*/\*+\s*", "", comment)
    inner = re.sub(r"\s*\*+/\s*\Z", "", inner)
    lines = inner.splitlines()

    kept: list[str] = []
    skipped_boilerplate = True
    for raw in lines:
        # Normalise leading "  * " or "  # " gutter
        stripped = raw.lstrip()
        if stripped.startswith("*") or stripped.startswith("#"):
            stripped = stripped[1:]
            if stripped.startswith(" "):
                stripped = stripped[1:]
        # Drop lines we re-emit
        if any(pat.match(raw) for pat in DROP_PATTERNS):
            continue
        inner = stripped.strip()
        # Drop pure-punctuation gutter remnants (lone *, #, --, etc.)
        if inner in ("", "*", "#", "--", "===", "—"):
            if kept and kept[-1].strip() != "*":
                kept.append(" *")
            continue
        kept.append(" * " + stripped.rstrip())
        skipped_boilerplate = False

    # Trim leading / trailing blank gutter lines
    while kept and kept[0].strip() in ("*", ""):
        kept.pop(0)
    while kept and kept[-1].strip() in ("*", ""):
        kept.pop()

    if not kept or skipped_boilerplate:
        return ""
    return "\n".join(kept)


def default_brief(rel_path: str) -> str:
    stem = Path(rel_path).stem
    return stem.replace("_", " ")


def strip_leading_comment(src: str) -> tuple[str, str]:
    """Return (existing_comment, body_without_comment)."""
    m = LEADING_COMMENT_RE.match(src)
    if not m:
        return "", src
    return m.group(0), src[m.end():]


def build_header(filename: str, group: str, brief: str, details_body: str) -> str:
    module, info, dependency = DIR_META.get(
        group,
        (group.replace("_", " ").title(), "", "")
    )
    head = TEMPLATE_HEAD.format(
        filename=filename,
        brief=brief or default_brief(filename),
        group=group,
        module=module or group,
        info=info or "—",
        dependency=dependency or "—",
    )
    if details_body:
        middle = TEMPLATE_DETAILS.format(details_body=details_body)
    else:
        middle = ""
    return head + middle + TEMPLATE_TAIL


def process_file(path: Path, root: Path, check: bool) -> bool:
    """Returns True iff the file was modified (or would be in --check)."""
    rel = path.relative_to(root).as_posix()
    if path.name in EXCLUDE_NAMES:
        return False

    raw = path.read_text(encoding="utf-8", errors="surrogateescape")
    existing, body = strip_leading_comment(raw)
    group = derive_group(rel)
    brief = derive_brief_from_existing(existing, default_brief(path.name))
    details_body = extract_details_body(existing)
    header = build_header(path.name, group, brief, details_body)
    new = header + "\n" + body.lstrip("\n")

    if new == raw:
        return False

    if check:
        print(f"would update: {rel}")
        return True

    path.write_text(new, encoding="utf-8", errors="surrogateescape")
    print(f"updated: {rel}")
    return True


def collect_files(root: Path) -> list[Path]:
    seen: set[Path] = set()
    files: list[Path] = []
    for pattern in INCLUDE_GLOBS:
        for p in root.glob(pattern):
            if not p.is_file():
                continue
            if p in seen:
                continue
            seen.add(p)
            files.append(p)
    return sorted(files)


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--check", action="store_true",
                   help="report files that would change, but do not modify them")
    p.add_argument("--root", type=Path, default=Path.cwd(),
                   help="repository root (default: cwd)")
    args = p.parse_args()

    root = args.root.resolve()
    files = collect_files(root)
    if not files:
        print("no files matched", file=sys.stderr)
        return 1

    changed = 0
    for path in files:
        if process_file(path, root, args.check):
            changed += 1
    print(f"\n{changed} / {len(files)} files {'would change' if args.check else 'modified'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
