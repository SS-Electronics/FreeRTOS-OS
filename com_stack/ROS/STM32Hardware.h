#ifndef _STM32_HARDWARE_H_
#define _STM32_HARDWARE_H_

/* Device drivers includes */
#include <bsw/drivers/device/com/usb/drv_usb.h>
#include <bsw/drivers/device/time/drv_time.h>
/*IPC include for inter-process data receive */
#include <bsw/ipc/mqueue/mqueue.h>
/* Kernel include */
#include <bsw/kernel/kernel.h>

/* Related configuration */
#include <app/bsw_config/arch_conf/board/board.h>

class STM32Hardware
{
public:
	STM32Hardware() {
	}

	void init()
	{
		/* Get the reference of the stream buffer */

	}

	// Read a byte of data from ROS connection.
	// If no data , returns -1
	int read()
	{
		uint8_t ch = -1;
#if(PERIPHERAL_USB_1_EN)
		uint32_t r;
		r = ringbuffer_getchar(&pipe_usb_1_drv_rx_handle, &ch);
#else
#error " ROS in included but not linked to any USB PERIPHERAL_USB_1_EN set to 1"
#endif
		if (1 == r)
			return ch;
		else
			return -1;
	}


	// Send a byte of data to ROS connection
	void write(uint8_t* data, int length)
	{
		/* Transmit data to USB driver */
		drv_usb_transmit(0, data, length);
	}

	// Returns milliseconds since start of program
	unsigned long time(void)
	{
		return drv_time_get_ticks();
	}

};

#endif

