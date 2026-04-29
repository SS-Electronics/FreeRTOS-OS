#!/usr/bin/env python3
"""
arm_dsp_gen.py — ARM CMSIS-DSP configuration code generator for FreeRTOS-OS
=============================================================================

Reads a DSP XML description (dsp_dev.xml) and generates two files in --outdir:

  dsp_config.h   — C header: CONFIG_ARM_DSP_* #defines for inclusion in firmware
  dsp_config.mk  — Makefile fragment: CONFIG_ARM_DSP_* variables for lib/Makefile

Usage
-----
  python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml
  python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml --outdir app/board

Makefile integration
--------------------
  make dsp-gen               # re-generate from XML, then compile
"""

import xml.etree.ElementTree as ET
import argparse
import sys
from datetime import date
from pathlib import Path


# ──────────────────────────────────────────────────────────────────────────────
#  Module table
#  name        : XML module name and macro suffix (upper-cased)
#  src_dir     : path relative to lib/CMSIS-DSP/Source/
#  aggregate   : aggregate .c file that compiles the whole module
# ──────────────────────────────────────────────────────────────────────────────

_DSP_MODULES = [
    {
        'name':      'common_tables',
        'src_dir':   'CommonTables',
        'aggregate': 'CommonTables.c',
        'macro':     'COMMON_TABLES',
        'desc':      'Shared FFT twiddle / sin-cos lookup tables',
    },
    {
        'name':      'basic_math',
        'src_dir':   'BasicMathFunctions',
        'aggregate': 'BasicMathFunctions.c',
        'macro':     'BASIC_MATH',
        'desc':      'add, sub, mult, abs, dot-product, scale, shift',
    },
    {
        'name':      'complex_math',
        'src_dir':   'ComplexMathFunctions',
        'aggregate': 'ComplexMathFunctions.c',
        'macro':     'COMPLEX_MATH',
        'desc':      'complex add, mult, abs, conjugate, magnitude, dot-product',
    },
    {
        'name':      'controller',
        'src_dir':   'ControllerFunctions',
        'aggregate': 'ControllerFunctions.c',
        'macro':     'CONTROLLER',
        'desc':      'PID controller, Clarke/Park transforms',
    },
    {
        'name':      'fast_math',
        'src_dir':   'FastMathFunctions',
        'aggregate': 'FastMathFunctions.c',
        'macro':     'FAST_MATH',
        'desc':      'arm_sin_f32, arm_cos_f32, arm_sqrt_f32 (table-driven)',
    },
    {
        'name':      'filtering',
        'src_dir':   'FilteringFunctions',
        'aggregate': 'FilteringFunctions.c',
        'macro':     'FILTERING',
        'desc':      'FIR, IIR/biquad, LMS, convolution, correlation, decimation',
    },
    {
        'name':      'interpolation',
        'src_dir':   'InterpolationFunctions',
        'aggregate': 'InterpolationFunctions.c',
        'macro':     'INTERPOLATION',
        'desc':      'linear, bilinear, spline interpolation',
    },
    {
        'name':      'matrix',
        'src_dir':   'MatrixFunctions',
        'aggregate': 'MatrixFunctions.c',
        'macro':     'MATRIX',
        'desc':      'matrix add, sub, mult, inverse, transpose, Cholesky',
    },
    {
        'name':      'statistics',
        'src_dir':   'StatisticsFunctions',
        'aggregate': 'StatisticsFunctions.c',
        'macro':     'STATISTICS',
        'desc':      'mean, power, RMS, variance, min, max, entropy',
    },
    {
        'name':      'support',
        'src_dir':   'SupportFunctions',
        'aggregate': 'SupportFunctions.c',
        'macro':     'SUPPORT',
        'desc':      'copy, fill, f32/q31/q15/q7 type-conversion utilities',
    },
    {
        'name':      'transform',
        'src_dir':   'TransformFunctions',
        'aggregate': 'TransformFunctions.c',
        'macro':     'TRANSFORM',
        'desc':      'CFFT, RFFT, DCT4 — requires common_tables',
    },
    {
        'name':      'bayes',
        'src_dir':   'BayesFunctions',
        'aggregate': 'BayesFunctions.c',
        'macro':     'BAYES',
        'desc':      'Naive Bayes classifier (float32)',
    },
    {
        'name':      'svm',
        'src_dir':   'SVMFunctions',
        'aggregate': 'SVMFunctions.c',
        'macro':     'SVM',
        'desc':      'SVM: linear, polynomial, RBF, sigmoid (float32)',
    },
    {
        'name':      'distance',
        'src_dir':   'DistanceFunctions',
        'aggregate': 'DistanceFunctions.c',
        'macro':     'DISTANCE',
        'desc':      'vector distance metrics (Minkowski, Canberra, Chebyshev, …)',
    },
    {
        'name':      'quaternion',
        'src_dir':   'QuaternionMathFunctions',
        'aggregate': 'QuaternionMathFunctions.c',
        'macro':     'QUATERNION',
        'desc':      'quaternion multiply, norm, inverse, conjugate, log, exp',
    },
    {
        'name':      'window',
        'src_dir':   'WindowFunctions',
        'aggregate': 'WindowFunctions.c',
        'macro':     'WINDOW',
        'desc':      'window functions (Bartlett, Hann, Hamming, Blackman, …)',
    },
]

_MODULE_BY_NAME = {m['name']: m for m in _DSP_MODULES}

_BANNER = """\
/*
 * AUTO-GENERATED — DO NOT EDIT
 * Generator : FreeRTOS-OS/scripts/arm_dsp_gen.py
 * Source    : {xml_path}
 * Date      : {date}
 *
 * Re-generate:
 *   python3 FreeRTOS-OS/scripts/arm_dsp_gen.py {xml_path}
 */
"""

_MK_BANNER = """\
# AUTO-GENERATED — DO NOT EDIT
# Generator : FreeRTOS-OS/scripts/arm_dsp_gen.py
# Source    : {xml_path}
# Date      : {date}
#
# Re-generate:
#   python3 FreeRTOS-OS/scripts/arm_dsp_gen.py {xml_path}
"""


# ──────────────────────────────────────────────────────────────────────────────
#  Parser
# ──────────────────────────────────────────────────────────────────────────────

class DspConfigParser:

    def __init__(self, xml_path: str):
        self.xml_path   = Path(xml_path)
        tree            = ET.parse(xml_path)
        root            = tree.getroot()

        self.target      = root.get('target',      'STM32F411xE')
        self.fpu         = root.get('fpu',          'cortex-m4')
        self.description = root.get('description',  '')

        # Build enabled-module set from XML
        modules_el = root.find('modules')
        if modules_el is None:
            modules_el = root

        self.enabled: dict[str, bool] = {}
        for el in modules_el.findall('module'):
            name    = el.get('name', '').strip().lower()
            enabled = el.get('enabled', 'no').strip().lower() in ('yes', '1', 'true')
            if name in _MODULE_BY_NAME:
                self.enabled[name] = enabled
            else:
                print(f'[arm_dsp_gen] WARNING: unknown module "{name}" in {xml_path}',
                      file=sys.stderr)

        # Default-disable any module not mentioned in the XML
        for m in _DSP_MODULES:
            self.enabled.setdefault(m['name'], False)

        any_enabled = any(self.enabled.values())
        self.dsp_enabled = any_enabled

    # ── public ────────────────────────────────────────────────────────────────

    def generate(self, outdir: str):
        out = Path(outdir)
        out.mkdir(parents=True, exist_ok=True)

        h_path  = out / 'dsp_config.h'
        mk_path = out / 'dsp_config.mk'

        h_path.write_text(self._gen_header())
        mk_path.write_text(self._gen_makefile_fragment())

        enabled_names = [m['name'] for m in _DSP_MODULES if self.enabled.get(m['name'])]
        print(f'[arm_dsp_gen] Target  : {self.target}  FPU: {self.fpu}')
        print(f'[arm_dsp_gen] Enabled : {", ".join(enabled_names) if enabled_names else "(none)"}')
        print(f'[arm_dsp_gen] -> {h_path}')
        print(f'[arm_dsp_gen] -> {mk_path}')

    # ── dsp_config.h ──────────────────────────────────────────────────────────

    def _gen_header(self) -> str:
        guard = 'BOARD_DSP_CONFIG_H_'
        L = []
        L.append(_BANNER.format(xml_path=self.xml_path, date=date.today()))
        L.append(f'#ifndef {guard}')
        L.append(f'#define {guard}')
        L.append('')
        L.append(f'/* DSP configuration for: {self.description} */')
        L.append(f'/* Target: {self.target}  FPU: {self.fpu} */')
        L.append('')

        dsp_val = '1' if self.dsp_enabled else '0'
        L.append(f'/* Master DSP enable — 1 when at least one module is active */')
        L.append(f'#define CONFIG_ARM_DSP_ENABLED          {dsp_val}')
        L.append('')

        L.append('/* ── Per-module enables ─────────────────────────────────────────────── */')
        for m in _DSP_MODULES:
            val   = '1' if self.enabled.get(m['name']) else '0'
            macro = f'CONFIG_ARM_DSP_{m["macro"]}'
            L.append(f'#define {macro:<38} {val}  /* {m["desc"]} */')

        L.append('')
        L.append(f'#endif /* {guard} */')
        L.append('')
        return '\n'.join(L)

    # ── dsp_config.mk ─────────────────────────────────────────────────────────

    def _gen_makefile_fragment(self) -> str:
        L = []
        L.append(_MK_BANNER.format(xml_path=self.xml_path, date=date.today()))
        L.append('')
        L.append(f'# Target: {self.target}  FPU: {self.fpu}')
        L.append('')

        dsp_val = '1' if self.dsp_enabled else '0'
        L.append(f'CONFIG_ARM_DSP_ENABLED          := {dsp_val}')
        L.append('')

        L.append('# Per-module enables')
        for m in _DSP_MODULES:
            val   = '1' if self.enabled.get(m['name']) else '0'
            macro = f'CONFIG_ARM_DSP_{m["macro"]}'
            L.append(f'{macro:<38} := {val}')

        L.append('')
        return '\n'.join(L)


# ──────────────────────────────────────────────────────────────────────────────
#  Entry point
# ──────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description='Generate dsp_config.h and dsp_config.mk from a DSP XML file.'
    )
    parser.add_argument('xml',
        help='Path to the DSP XML file (e.g. app/board/dsp_dev.xml)')
    parser.add_argument('--outdir', '-o',
        help='Output directory for generated files  '
             '(default: same directory as the XML file)')
    args = parser.parse_args()

    xml_path = Path(args.xml)
    if not xml_path.exists():
        print(f'Error: XML file not found: {xml_path}', file=sys.stderr)
        sys.exit(1)

    gen    = DspConfigParser(str(xml_path))
    outdir = Path(args.outdir) if args.outdir else xml_path.parent

    gen.generate(str(outdir))


if __name__ == '__main__':
    main()
