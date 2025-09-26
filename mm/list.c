/**
 * @file    list.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   all link list operation definitions
 * @details Linklist is the base of all objects dynamically created
 *          in kernel.
 * @date    2025-09-26
 *
 * @note This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>.
 */


#include "mm/list.h"



/**
 * @brief Initialize a list node [ Doubly circular].
 * @note  Not need to be called at init. 
 *        using INIT_LIST_NODE or LIST_NODE_INIT
 *        we can init at compile time
 * @param list Pointer to the list node to initialize.
 */
__always_inline void list_init( struct list_node * list)
{
    /** Circular list init with its own 
     * pointer as blank value 
     */
    list->next  =   list;
    list->prev  =   list;
}



/**
 * @brief add a node in between two nodes.
 * @note  This is a gneeric function adding 
 *        a node in between two nodes
 * @param new Pointer to the node that need to be added
 * @param prev Pointer to the previous node
 * @param next Pointer to the next node
 */
__always_inline void list_add( struct list_node *new,
                        struct list_node *prev,
                        struct list_node *next)
{
    /**  Check for NULL because its a doubly circular
     *  linked list 
     */
    if( (new != NULL) && 
        (next != NULL) &&
        (prev != NULL)
      )
    {
        next->prev = new;
        prev->next = new;

        new->prev = prev;
        new->next = next;
    }
}



/**
 * @brief add a node in the head.
 * @note  This wraper function add node in the head of circular
 *        linked list
 * @param new Pointer to the node that need to be added
 * @param head Pointer to the initialize node instance
 */
__always_inline void list_add_head( struct list_node *new,
                        struct list_node *head)
{
    list_add(new, head, head->next);
}



/**
 * @brief add a node in the tail.
 * @note  This wraper function add node in the head of circular
 *        linked list
 * @param new Pointer to the node that need to be added
 * @param head Pointer to the initialize node instance
 */
__always_inline void list_add_tail( struct list_node *new,
                        struct list_node *head)
{
    list_add(new, head->prev, head);
}



/**
 * @brief Delete an item from the linked list.
 * @note  This wraper function add node in the head of circular
 *        linked list
 * @param new Pointer to the node that need to be added
 * @param head Pointer to the initialize node instance
 */
__always_inline void list_delete( struct list_node * node)
{
    /** prev node's next -> current node next */
    node->prev->next = node->next;

    /** next node's prev -> curr node's prev */
    node->next->prev = node->prev;
}

