/*
File:        list.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       kernel/mm
Info:         all linked list operation functions           
Dependency:   

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation, either version 
3 of the License, or (at your option) any later version.

FreeRTOS-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>. */

#include "mm/list.h"



/* *****************************************************
 * Circular doubly linked list implementation.
 * *****************************************************/

/* *****************************************************
 * 
 * List init
 *
 * *****************************************************/
__always_inline void list_init( struct list_node * list)
{
    /*  Init a doubly linked list */
    list->next  =   list;
    list->prev  =   list;
}

/* *****************************************************
 * 
 * List add [ already know the prev and next and 
 * place dinbetween]
 *
 * *****************************************************/
__always_inline int list_add( struct list_node *new,
                        struct list_node *next,
                        struct list_node *prev0)
{
    /* Check for NULL */
    if(   )

}