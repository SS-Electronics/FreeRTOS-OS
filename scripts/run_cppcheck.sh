#!/usr/bin/env bash
# run_cppcheck.sh
# CPPcheck + MISRA C:2012 static analysis for FreeRTOS-OS (STM32F411xE).
#
# Usage:
#   ./scripts/run_cppcheck.sh [OPTIONS]
#
# Options:
#   --misra              Enable MISRA C:2012 checks (addon)
#   --misra-rules=<file> Path to MISRA rule-text file (unlocks descriptions)
#   --output-dir=<dir>   Report output directory (default: reports/cppcheck)
#   --severity=<level>   Minimum severity: error|warning|style|performance|
#                        portability|information (default: warning)
#   --html               Generate HTML report (requires cppcheck-htmlreport)
#   --jobs=<n>           Parallel analysis jobs (default: nproc)
#   --verbose            Show cppcheck invocation and file list
#   -h, --help           Show this help
#
# Exit codes:
#   0  No issues found (at or above severity threshold)
#   1  Issues found
#   2  Script/tool error
#
# Examples:
#   ./scripts/run_cppcheck.sh
#   ./scripts/run_cppcheck.sh --misra --html
#   ./scripts/run_cppcheck.sh --misra --misra-rules=/opt/misra_c2012.txt --severity=error

set -euo pipefail

# ── Script location (all paths are relative to project root) ─────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ── Defaults ──────────────────────────────────────────────────────────────────
ENABLE_MISRA=0
MISRA_RULES_FILE=""
OUTPUT_DIR="${PROJECT_ROOT}/reports/cppcheck"
SEVERITY="warning"
ENABLE_HTML=0
JOBS="$(nproc 2>/dev/null || echo 4)"
VERBOSE=0
ADDON_JSON="${SCRIPT_DIR}/misra_addon.json"
SUPPRESSIONS_FILE="${SCRIPT_DIR}/cppcheck_suppressions.txt"

# ── Colour helpers ────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
success() { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
error()   { echo -e "${RED}[ERR]${RESET}   $*" >&2; }
header()  { echo -e "\n${BOLD}${CYAN}══════════════════════════════════════${RESET}"; \
            echo -e "${BOLD}${CYAN}  $*${RESET}"; \
            echo -e "${BOLD}${CYAN}══════════════════════════════════════${RESET}"; }

# ── Help ──────────────────────────────────────────────────────────────────────
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

  --misra               Enable MISRA C:2012 checks
  --misra-rules=<file>  MISRA rule-text file (for human-readable descriptions)
  --output-dir=<dir>    Report directory  [default: reports/cppcheck]
  --severity=<level>    error|warning|style|performance|portability|information
                        [default: warning]
  --html                Generate HTML report via cppcheck-htmlreport
  --jobs=<n>            Parallel jobs  [default: $(nproc)]
  --verbose             Print full cppcheck invocation
  -h, --help            Show this help

MISRA rule-text file:
  The MISRA C:2012 document is commercially licensed. If you own a copy,
  export the rules as plain text (one rule per line, e.g. "Rule 1.1 ...") and
  pass the path via --misra-rules=. Without it, violation IDs are shown instead
  of descriptions (e.g. misra-c2012-8.1).

EOF
}

# ── Argument parsing ──────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --misra)             ENABLE_MISRA=1 ;;
        --misra-rules=*)     MISRA_RULES_FILE="${arg#*=}" ;;
        --output-dir=*)      OUTPUT_DIR="${arg#*=}" ;;
        --severity=*)        SEVERITY="${arg#*=}" ;;
        --html)              ENABLE_HTML=1 ;;
        --jobs=*)            JOBS="${arg#*=}" ;;
        --verbose)           VERBOSE=1 ;;
        -h|--help)           usage; exit 0 ;;
        *) warn "Unknown argument: $arg"; usage; exit 2 ;;
    esac
done

# ── Pre-flight checks ─────────────────────────────────────────────────────────
if ! command -v cppcheck &>/dev/null; then
    error "cppcheck not found. Run: ./scripts/install_cppcheck.sh"
    exit 2
fi

CPPCHECK_VER=$(cppcheck --version | grep -oP '\d+\.\d+' | head -1)
info "cppcheck version: ${CPPCHECK_VER}"

if [[ "$ENABLE_HTML" -eq 1 ]] && ! command -v cppcheck-htmlreport &>/dev/null; then
    warn "cppcheck-htmlreport not found — HTML report disabled."
    ENABLE_HTML=0
fi

if [[ -n "$MISRA_RULES_FILE" ]] && [[ ! -f "$MISRA_RULES_FILE" ]]; then
    error "MISRA rules file not found: ${MISRA_RULES_FILE}"
    exit 2
fi

# ── Source file collection (project code only, no vendor) ─────────────────────
header "Collecting source files"

# Directories containing project-owned C code
PROJECT_SRC_DIRS=(
    "${PROJECT_ROOT}/drivers"
    "${PROJECT_ROOT}/drv_app"
    "${PROJECT_ROOT}/drv_ext_chips"
    "${PROJECT_ROOT}/services"
    "${PROJECT_ROOT}/shell"
    "${PROJECT_ROOT}/ipc"
    "${PROJECT_ROOT}/mm"
    "${PROJECT_ROOT}/irq"
    "${PROJECT_ROOT}/init"
    "${PROJECT_ROOT}/lib"
    "${PROJECT_ROOT}/boot"
    "${PROJECT_ROOT}/net"
    "${PROJECT_ROOT}/com_stack"
)

FILE_LIST=""
TOTAL_FILES=0
for dir in "${PROJECT_SRC_DIRS[@]}"; do
    if [[ -d "$dir" ]]; then
        while IFS= read -r -d '' f; do
            FILE_LIST+="${f}"$'\n'
            TOTAL_FILES=$((TOTAL_FILES + 1))
        done < <(find "$dir" -name "*.c" -not -path "*/build/*" -print0 2>/dev/null)
    fi
done

if [[ "$TOTAL_FILES" -eq 0 ]]; then
    error "No .c source files found under project source directories."
    exit 2
fi

info "Found ${TOTAL_FILES} source file(s) to analyse."

# Write file list to a temp file for cppcheck --file-list
# Temp files (cleaned up on exit).
# Stray .dump files in the source tree are also removed on exit so nothing is
# left behind if the script is interrupted mid-MISRA-pass.
FILE_LIST_TMP=$(mktemp /tmp/cppcheck_files_XXXX.txt)
SUPPRESS_TMP=$(mktemp /tmp/cppcheck_suppress_XXXX.txt)
cleanup() {
    rm -f "$FILE_LIST_TMP" "$SUPPRESS_TMP"
    # Remove cppcheck intermediate files scattered in source dirs:
    #   *.dump      — AST/symbol dump used by the MISRA addon
    #   *.ctu-info  — Cross Translation Unit analysis state (generated by -j + --max-ctu-depth)
    find "$PROJECT_ROOT" -maxdepth 7 \
        \( -name "*.dump" -o -name "*.ctu-info" \) \
        -not -path "*/reports/*" \
        -not -path "*/build/*" \
        -delete 2>/dev/null || true
    # Clean up DUMP_DIR itself (intermediate artefacts, not committed)
    [[ -d "${OUTPUT_DIR}/dumps" ]] && rm -rf "${OUTPUT_DIR}/dumps"
}
trap cleanup EXIT

echo -n "$FILE_LIST" > "$FILE_LIST_TMP"

# Strip comments and blank lines — cppcheck 2.x does not accept them
grep -v '^\s*#' "$SUPPRESSIONS_FILE" | grep -v '^\s*$' > "$SUPPRESS_TMP" || true

if [[ "$VERBOSE" -eq 1 ]]; then
    info "Source files:"
    echo "$FILE_LIST" | sed 's|^|  |'
fi

# ── Include paths ─────────────────────────────────────────────────────────────
INCLUDES=(
    "-I${PROJECT_ROOT}"
    "-I${PROJECT_ROOT}/include"
    "-I${PROJECT_ROOT}/config"
    "-I${PROJECT_ROOT}/drivers"
    "-I${PROJECT_ROOT}/include/services"
    "-I${PROJECT_ROOT}/include/drv_app"
    "-I${PROJECT_ROOT}/include/shell"
    "-I${PROJECT_ROOT}/lib"
    "-I${PROJECT_ROOT}/arch/arm/CMSIS_6"
    "-I${PROJECT_ROOT}/arch/devices"
    "-I${PROJECT_ROOT}/arch/devices/STM"
    "-I${PROJECT_ROOT}/arch/devices/device_conf"
    "-I${PROJECT_ROOT}/arch/devices/STM/STM32F4xx"
    "-I${PROJECT_ROOT}/arch/devices/STM/stm32f4xx-hal-driver/Inc"
    "-I${PROJECT_ROOT}/kernel/FreeRTOS-Kernel/include"
    "-I${PROJECT_ROOT}/kernel/FreeRTOS-Plus-CLI"
    "-I${PROJECT_ROOT}/kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM4F"
)

# ── Preprocessor defines ──────────────────────────────────────────────────────
DEFINES=(
    "-DSTM32F411xE"
    "-DUSE_HAL_DRIVER"
    "-DARM_MATH_CM4"
    "-D__FPU_PRESENT=1"
    "-D__FPU_USED=1"
    "-DCORTEX_M4"
    # GCC sets __ARM_ARCH_PROFILE automatically to 'M' (ASCII 77) for Cortex-M.
    # cppcheck does not; without it cmsis_gcc.h hits #error "Unknown Arm architecture profile".
    "-D__ARM_ARCH_PROFILE=77"
    "-DCONFIG_DEFAULT_DEBUG_EN=1"
    "-DCONFIG_DRV_DEBUG_EN=1"
    "-DCONFIG_HAL_UART_MODULE_ENABLED=1"
    "-DCONFIG_HAL_SPI_MODULE_ENABLED=1"
    "-DCONFIG_HAL_I2C_MODULE_ENABLED=1"
    "-DCONFIG_HAL_GPIO_MODULE_ENABLED=1"
    "-DCONFIG_HAL_DMA_MODULE_ENABLED=1"
    "-DCONFIG_HAL_RCC_MODULE_ENABLED=1"
    "-DCONFIG_HAL_TIM_MODULE_ENABLED=1"
    "-DCONFIG_INC_SERVICE_UART_MGMT=1"
    "-DCONFIG_INC_SERVICE_IIC_MGMT=1"
    "-DCONFIG_INC_SERVICE_OS_SHELL_MGMT=1"
)

# ── Output directory setup ────────────────────────────────────────────────────
mkdir -p "$OUTPUT_DIR"
REPORT_XML="${OUTPUT_DIR}/cppcheck_report.xml"
REPORT_TXT="${OUTPUT_DIR}/cppcheck_report.txt"
MISRA_REPORT_TXT="${OUTPUT_DIR}/misra_report.txt"
DUMP_DIR="${OUTPUT_DIR}/dumps"

# ── Build cppcheck command ────────────────────────────────────────────────────
header "Running CPPcheck"

# Map severity threshold to --enable= flags.
# cppcheck always emits errors; the flags below add checks at higher levels.
case "$SEVERITY" in
    error)       ENABLE_FLAGS="--enable=warning" ;;   # run all, exit code on error only
    warning)     ENABLE_FLAGS="--enable=warning" ;;
    style)       ENABLE_FLAGS="--enable=warning,style" ;;
    performance) ENABLE_FLAGS="--enable=warning,style,performance" ;;
    portability) ENABLE_FLAGS="--enable=warning,style,performance,portability" ;;
    information) ENABLE_FLAGS="--enable=all" ;;
    *)           warn "Unknown severity '${SEVERITY}', defaulting to warning."
                 ENABLE_FLAGS="--enable=warning" ;;
esac

CPPCHECK_ARGS=(
    "$ENABLE_FLAGS"
    "--std=c99"
    "--platform=${PROJECT_ROOT}/scripts/arm_cm4.xml"
    "--language=c"
    "--max-ctu-depth=4"
    "--error-exitcode=1"
    "--suppressions-list=${SUPPRESS_TMP}"
    "--inline-suppr"
    "--force"
    "-j${JOBS}"
    "--file-list=${FILE_LIST_TMP}"
)

# Platform: use bundled arm32-wchar_t4.xml if our custom one is absent
ARM_PLATFORM_CUSTOM="${PROJECT_ROOT}/scripts/arm_cm4.xml"
ARM_PLATFORM_BUNDLED="/usr/lib/x86_64-linux-gnu/cppcheck/platforms/arm32-wchar_t4.xml"

if [[ -f "$ARM_PLATFORM_CUSTOM" ]]; then
    CPPCHECK_ARGS=("${CPPCHECK_ARGS[@]/--platform=*${PROJECT_ROOT}*/--platform=${ARM_PLATFORM_CUSTOM}}")
elif [[ -f "$ARM_PLATFORM_BUNDLED" ]]; then
    # Replace platform argument
    UPDATED_ARGS=()
    for a in "${CPPCHECK_ARGS[@]}"; do
        if [[ "$a" == --platform=* ]]; then
            UPDATED_ARGS+=("--platform=${ARM_PLATFORM_BUNDLED}")
        else
            UPDATED_ARGS+=("$a")
        fi
    done
    CPPCHECK_ARGS=("${UPDATED_ARGS[@]}")
else
    # Remove platform argument entirely
    UPDATED_ARGS=()
    for a in "${CPPCHECK_ARGS[@]}"; do
        [[ "$a" == --platform=* ]] || UPDATED_ARGS+=("$a")
    done
    CPPCHECK_ARGS=("${UPDATED_ARGS[@]}")
    warn "No ARM platform file found — analysis proceeds without platform model."
fi

# Append includes and defines — CPPCHECK_ARGS is now final for analysis passes
CPPCHECK_ARGS+=("${INCLUDES[@]}" "${DEFINES[@]}")

if [[ "$VERBOSE" -eq 1 ]]; then
    info "cppcheck invocation:"
    echo "  cppcheck ${CPPCHECK_ARGS[*]}" | fold -s -w 100 | sed 's/^/    /'
fi

# ── Run cppcheck (XML output to file, human text to terminal) ─────────────────
info "Analysing ${TOTAL_FILES} files with severity >= ${SEVERITY} ..."
info "XML report  → ${REPORT_XML}"
info "Text report → ${REPORT_TXT}"

CPPCHECK_EXIT=0

# Text output (human-readable) to terminal + text file
cppcheck "${CPPCHECK_ARGS[@]}" \
    "--template=gcc" \
    2>&1 | tee "$REPORT_TXT" || CPPCHECK_EXIT=$?

# XML output (for IDE / CI integration)
cppcheck "${CPPCHECK_ARGS[@]}" \
    "--xml" \
    2>"$REPORT_XML" || true

# ── MISRA C:2012 checks ───────────────────────────────────────────────────────
# Dump generation is a SEPARATE pass so it never pollutes CPPCHECK_ARGS used
# by the text/XML analysis passes above.
MISRA_EXIT=0
if [[ "$ENABLE_MISRA" -eq 1 ]]; then
    header "Running MISRA C:2012 Checks"

    # Locate misra.py
    MISRA_PY=""
    for candidate in \
        "/usr/lib/x86_64-linux-gnu/cppcheck/addons/misra.py" \
        "/usr/share/cppcheck/addons/misra.py" \
        "/usr/local/share/cppcheck/addons/misra.py"; do
        [[ -f "$candidate" ]] && { MISRA_PY="$candidate"; break; }
    done

    if [[ -z "$MISRA_PY" ]]; then
        error "misra.py not found. Run ./scripts/install_cppcheck.sh first."
        MISRA_EXIT=2
    else
        info "Using MISRA addon: ${MISRA_PY}"
        info "MISRA report    → ${MISRA_REPORT_TXT}"
        mkdir -p "$DUMP_DIR"

        # Step 1: Generate dump files (dedicated pass, stdout/stderr discarded)
        info "Generating .dump files for MISRA analysis ..."
        cppcheck "${CPPCHECK_ARGS[@]}" \
            "--dump" \
            >/dev/null 2>&1 || true

        # Collect .dump files from source dirs and move to DUMP_DIR
        find "$PROJECT_ROOT" -maxdepth 7 -name "*.dump" \
            -not -path "*/build/*" \
            -not -path "*/reports/*" \
            2>/dev/null | while IFS= read -r dmp; do
            mv "$dmp" "$DUMP_DIR/" 2>/dev/null || true
        done

        DUMP_FILES=()
        while IFS= read -r -d '' d; do
            DUMP_FILES+=("$d")
        done < <(find "$DUMP_DIR" -name "*.dump" -print0 2>/dev/null)

        if [[ ${#DUMP_FILES[@]} -eq 0 ]]; then
            warn "No .dump files produced. MISRA analysis skipped."
            MISRA_EXIT=2
        else
            info "Running MISRA checks on ${#DUMP_FILES[@]} dump file(s) ..."

            # Step 2: Run misra.py on the dump files — text report
            MISRA_ADDON_ARGS=("$MISRA_PY" "--cli")
            [[ -n "$MISRA_RULES_FILE" ]] && MISRA_ADDON_ARGS+=("--rule-texts=${MISRA_RULES_FILE}")
            MISRA_ADDON_ARGS+=("${DUMP_FILES[@]}")

            python3 "${MISRA_ADDON_ARGS[@]}" 2>&1 | tee "$MISRA_REPORT_TXT" || MISRA_EXIT=$?

            # Step 3: Run cppcheck --addon=misra for XML-format MISRA report
            MISRA_XML="${OUTPUT_DIR}/misra_cppcheck.xml"
            if [[ -n "$MISRA_RULES_FILE" ]]; then
                MISRA_JSON_TMP=$(mktemp /tmp/misra_addon_XXXX.json)
                trap 'rm -f "$FILE_LIST_TMP" "$SUPPRESS_TMP" "$MISRA_JSON_TMP"' EXIT
                printf '{"script":"%s","args":["--rule-texts=%s","--no-summary"]}\n' \
                    "$MISRA_PY" "$MISRA_RULES_FILE" > "$MISRA_JSON_TMP"
                cppcheck "${CPPCHECK_ARGS[@]}" \
                    "--addon=${MISRA_JSON_TMP}" "--xml" \
                    2>"$MISRA_XML" || true
            else
                cppcheck "${CPPCHECK_ARGS[@]}" \
                    "--addon=${ADDON_JSON}" "--xml" \
                    2>"$MISRA_XML" || true
            fi
            info "MISRA XML       → ${MISRA_XML}"
        fi
    fi
fi

# ── HTML report ───────────────────────────────────────────────────────────────
if [[ "$ENABLE_HTML" -eq 1 ]]; then
    header "Generating HTML Report"
    HTML_DIR="${OUTPUT_DIR}/html"
    mkdir -p "$HTML_DIR"

    if cppcheck-htmlreport \
        --file="$REPORT_XML" \
        --report-dir="$HTML_DIR" \
        --source-dir="${PROJECT_ROOT}" 2>/dev/null; then
        success "HTML report → ${HTML_DIR}/index.html"
    else
        warn "cppcheck-htmlreport failed. Check XML report: ${REPORT_XML}"
    fi

    if [[ "$ENABLE_MISRA" -eq 1 ]] && [[ -f "${OUTPUT_DIR}/misra_cppcheck.xml" ]]; then
        MISRA_HTML_DIR="${OUTPUT_DIR}/misra_html"
        mkdir -p "$MISRA_HTML_DIR"
        if cppcheck-htmlreport \
            --file="${OUTPUT_DIR}/misra_cppcheck.xml" \
            --report-dir="$MISRA_HTML_DIR" \
            --source-dir="${PROJECT_ROOT}" 2>/dev/null; then
            success "MISRA HTML → ${MISRA_HTML_DIR}/index.html"
        fi
    fi
fi

# ── Summary ───────────────────────────────────────────────────────────────────
header "Analysis Summary"

issue_count() {
    local file="$1"
    [[ -f "$file" ]] || { echo 0; return; }
    grep -c "severity=" "$file" 2>/dev/null || echo 0
}

errors=$(grep -c " error:"   "$REPORT_TXT" 2>/dev/null || true); errors=${errors:-0}
warns=$(grep  -c " warning:" "$REPORT_TXT" 2>/dev/null || true); warns=${warns:-0}
styles=$(grep -c " style:"   "$REPORT_TXT" 2>/dev/null || true); styles=${styles:-0}

echo -e "  ${BOLD}Source files analysed :${RESET} ${TOTAL_FILES}"
echo -e "  ${BOLD}CPPcheck errors       :${RESET} ${RED}${errors}${RESET}"
echo -e "  ${BOLD}CPPcheck warnings     :${RESET} ${YELLOW}${warns}${RESET}"
echo -e "  ${BOLD}CPPcheck style notes  :${RESET} ${styles}"

if [[ "$ENABLE_MISRA" -eq 1 ]] && [[ -f "$MISRA_REPORT_TXT" ]]; then
    misra_issues=$(grep -c "misra" "$MISRA_REPORT_TXT" 2>/dev/null || true); misra_issues=${misra_issues:-0}
    echo -e "  ${BOLD}MISRA C:2012 issues   :${RESET} ${YELLOW}${misra_issues}${RESET}"
fi

echo ""
echo -e "  ${BOLD}Reports saved to: ${CYAN}${OUTPUT_DIR}/${RESET}"
echo ""

# ── Final exit code ───────────────────────────────────────────────────────────
FINAL_EXIT=0
[[ "$CPPCHECK_EXIT" -ne 0 ]] && FINAL_EXIT=1
[[ "$MISRA_EXIT" -eq 1 ]]   && FINAL_EXIT=1

if [[ "$FINAL_EXIT" -eq 0 ]]; then
    success "Analysis complete — no issues found above severity '${SEVERITY}'."
else
    warn "Analysis complete — issues found. Review reports in: ${OUTPUT_DIR}/"
fi

exit "$FINAL_EXIT"
