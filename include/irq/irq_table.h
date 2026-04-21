/*
 * irq_table.h — Static IRQ descriptor table declarations
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * irq_table.c holds a human-readable name for every IRQ ID defined in
 * irq_notify.h.  The table is the canonical list of all software IRQ IDs
 * in the system and is intended to be auto-generated from a hardware
 * description file when the peripheral set grows.
 */

#ifndef INCLUDE_IRQ_IRQ_TABLE_H_
#define INCLUDE_IRQ_IRQ_TABLE_H_

#include <irq/irq_notify.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Return the human-readable name for an IRQ ID.
 * @return Static string (never NULL). Returns "INVALID" for out-of-range IDs
 *         and "UNKNOWN" for IDs with no assigned name.
 */
const char *irq_table_get_name(irq_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_IRQ_IRQ_TABLE_H_ */
