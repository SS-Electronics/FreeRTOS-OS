/**
 * @file    list.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   All linked list operation functions definitions and macros.
 * @details Part of the kernel/mm module. Provides utility functions for 
 *          managing link list of the objects.
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

#ifndef __MM_LIST__
#define __MM_LIST__

#include "def_std.h"



/**
 * @section Data type definitions
 * @brief   This section contains all data structure 
 *          definitions used in link list.
 */

/**
 * @struct list_node
 * @brief Maintain each node
 */
struct list_node 
{
    struct list_node *next; /**> Pointer holding next node */
    struct list_node *prev; /**> Pointer holding previous node */
};

/**
 * @struct list_head
 * @brief  Maintain the head
 * @note   Use in special case 
 *         otherwise the previous data 
 */
struct list_head 
{
    struct list_head * head;    /**> Pointer holding the head node */
};



/**
 * @section Macro definiitons
 * @brief   Init functions, and data operators
 */

/**
 * @brief  initialize a list
 * @param name name of the list instance
 */
#define LIST_NODE_INIT(name) { &(name), &(name) }


/**
 * @brief  declare and initialize the list instance
 * @param name name of the list instance
 */
#define LIST_NODE_HEAD(name) \
    struct list_head name = LIST_NODE_INIT(name)


/**
 * @brief  declare and initialize the list instance
 * @param pointer pointer to the initialized list instance
 */
#define INIT_LIST_NODE(ptr) \
    do { \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
    } while (0)


/**
 * @brief Converts a pointer to a struct member (ptr) 
 *        into a pointer to the containing struct (type).
 * @param ptr Pointer to a struct member inside some struct
 * @param type Type of the container struct
 * @param member Name of the member within the struct 
 *               that ptr points to
 * @note  (type *)0
 *         Creates a null pointer of the container struct type.
 *        We use it to calculate the offset of the member within 
 *         the struct without accessing memory.
 *
 *         ((type *)0)->member
 *         Refers to the member field in the struct at address 0.
 *         Its type is used in typeof to declare __mptr.
 *
 *        __mptr = ptr
 *        Stores the pointer to the member we actually have.
 *
 *        offsetof(type, member)
 *        Computes the byte offset of member inside type.
 *        (char *)__mptr - offsetof(type, member)
 *
 *        Subtract the offset from the member pointer to get the base address of the struct.
 *        Cast to (type *)
 *        Converts the address back to a pointer of the container struct. 
 */
#define container_of(ptr, type, member) \
    ({ \
    const typeof( ( (type *)0 )->member ) *__mptr = (ptr); \ 
    (type *)((char *)__mptr - offsetof(type, member)); \
    }) 


/**
 * @brief find the list entry
 * @param ptr Pointer to a struct member inside some struct
 * @param type Type of the container struct
 * @param member Name of the member within the struct 
 *               that ptr points to
 */   
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)


/**
 * @brief Find the first entry in the list
 * @param ptr Pointer to a struct member inside some struct
 * @param type Type of the container struct
 * @param member Name of the member within the struct 
 *               that ptr points to
 * 
 * @note Head-> next is the first entry in the list  
 */    
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)


 /**
 * @brief  Iteration through the complete list
 * @param pos Pointer to the container structure
 * @param member Name of the struct list_head 
 *              field inside the container struct
 */       
#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

 /**
 * @brief  check the list is empty or not
 * @param head list head
 * @note If the list next is the head itself then there 
 */  
#define list_empty(head) ((head)->next == (head))


/**
 * @brief  Iteration through the complete list
 * @param pos Pointer to the container structure
 * @param head Pointer to the struct list_head 
 *              representing the list head
 * @param member Name of the struct list_head 
 *              field inside the container struct
 */
#define list_for_each_entry(pos, head, member) \
    for (pos = list_first_entry(head, typeof(*pos), member); \
        &pos->member != (head); \
        pos = list_next_entry(pos, member))







/**
 * @defgroup list_api Linked List API
 * @brief Functions to manipulate linked lists.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a list node [ Doubly circular]
 * @param list Pointer to the list node to initialize.
 */
void list_init(struct list_node *list);

/**
 * @brief Add a new node between two existing nodes.
 * @param new Pointer to the new node to add.
 * @param prev Pointer to the previous node.
 * @param next Pointer to the next node.
 */
void list_add(struct list_node *new,
              struct list_node *prev,
              struct list_node *next);

/**
 * @brief Add a new node at the head of the list.
 * @param new Pointer to the new node to add.
 * @param head Pointer to the head node of the list.
 */
void list_add_head(struct list_node *new,
                   struct list_node *head);

/**
 * @brief Add a new node at the tail of the list.
 * @param new Pointer to the new node to add.
 * @param head Pointer to the head node of the list.
 */
void list_add_tail(struct list_node *new,
                   struct list_node *head);

/**
 * @brief Delete a node from the list.
 * @param node Pointer to the node to delete.
 */
void list_delete(struct list_node *node);

#ifdef __cplusplus
}
#endif

/** @} */ // end of list_api








#endif