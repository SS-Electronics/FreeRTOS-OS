/*
 * os_config.h — shim that re-exports conf_os.h
 *
 * uart_mgmt.h and other service headers include <config/os_config.h>.
 * All OS configuration lives in conf_os.h; this file simply forwards it.
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef FREERTOS_OS_INCLUDE_CONFIG_OS_CONFIG_H_
#define FREERTOS_OS_INCLUDE_CONFIG_OS_CONFIG_H_

#include <conf_os.h>

#endif /* FREERTOS_OS_INCLUDE_CONFIG_OS_CONFIG_H_ */
