/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS OS project
#
# RTOS OS project is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS OS project is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/


#include <os/kernel_syscall.h>


static char temp_time_buffer[10]; // Max value of uint32_t
static char temp_char_buffer[CONF_MAX_CHAR_IN_PRINTK]; // Max value of uint32_t




/* *****************************************************
 *
 *
 *
 * *****************************************************/
int32_t	printk(char* ch)
{
#if (NO_OF_UART > 0)

	ATOMIC_ENTER_CRITICAL();

	int 	len, DataIdx;
	char 	temp_byte_buff;

	memset(temp_time_buffer, 0, 10);
	memset(temp_char_buffer, 0, CONF_MAX_CHAR_IN_PRINTK);

	/*
	 * Print the time-stamp
	 * */

	sprintf(temp_time_buffer,"[%d] ",(int)drv_time_get_ticks());
	len = strlen((char*)temp_time_buffer);

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		temp_byte_buff = temp_time_buffer[DataIdx];
		ringbuffer_putchar(&ipc_handle_printk_buffer, temp_byte_buff);
	}

	/*
	 * Print actual message
	 * */
	len = strlen(ch);

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		temp_byte_buff = ch[DataIdx];
		ringbuffer_putchar(&ipc_handle_printk_buffer, temp_byte_buff);
	}

	ATOMIC_EXIT_CRITICAL();
#endif
}


/* Variables */
extern int __io_putchar(int ch) __attribute__((weak));
extern int __io_getchar(void) __attribute__((weak));


char *__env[1] = { 0 };
char **environ = __env;


/* Functions */
void initialise_monitor_handles()
{
}

int _getpid(void)
{
  return 1;
}

int _kill(int pid, int sig)
{
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

void _exit (int status)
{
  _kill(status, -1);
  while (1) {}    /* Make sure we hang here */
}

int _read(int file, char *ptr, int len)
{
  (void)file;
  int DataIdx;

  for (DataIdx = 0; DataIdx < len; DataIdx++)
  {
    *ptr++ = __io_getchar();
  }

  return len;
}

int _write(int file, char *ptr, int len)
{
  (void)file;
  int DataIdx;

  for (DataIdx = 0; DataIdx < len; DataIdx++)
  {
    __io_putchar(*ptr++);
  }
  return len;
}

int _close(int file)
{
  (void)file;
  return -1;
}


int _fstat(int file, struct stat *st)
{
  (void)file;
//  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file)
{
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir)
{
  (void)file;
  (void)ptr;
  (void)dir;
  return 0;
}

int _open(char *path, int flags, ...)
{
  (void)path;
  (void)flags;
  /* Pretend like we always fail */
  return -1;
}

int _wait(int *status)
{
  (void)status;
//  errno = ECHILD;
  return -1;
}

int _unlink(char *name)
{
  (void)name;
//  errno = ENOENT;
  return -1;
}

int _times(struct tms *buf)
{
  (void)buf;
  return -1;
}

int _stat(char *file, struct stat *st)
{
  (void)file;
//  st->st_mode = S_IFCHR;
  return 0;
}

int _link(char *old, char *new)
{
  (void)old;
  (void)new;
//  errno = EMLINK;
  return -1;
}

int _fork(void)
{
//  errno = EAGAIN;
  return -1;
}

int _execve(char *name, char **argv, char **env)
{
  (void)name;
  (void)argv;
  (void)env;
//  errno = ENOMEM;
  return -1;
}




