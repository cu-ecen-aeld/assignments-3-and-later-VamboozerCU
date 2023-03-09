/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#include "aesd-circular-buffer.h"

//#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct aesd_circular_bufferK{
    // An array of pointers to memory allocated for the most recent write operations
    struct aesd_buffer_entry *entry[AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
    uint8_t in_offs; // The current location in the entry structure where the next 
                     // write should be stored.
    uint8_t out_offs; // The start location in the entry structure to read from
    bool full; // set to true when the buffer entry structure is full
};

struct aesd_dev
{
    struct cdev cdev;     /* Char device structure      */
    struct mutex my_mutex;
    struct aesd_circular_bufferK circular_buffer;
    char* currentWriteBuffer;
    ssize_t currentWriteBuffer_Size;
};

void aesd_add_entryK(struct aesd_circular_bufferK *buffer, const struct aesd_buffer_entry *add_entry);
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fposK(struct aesd_circular_bufferK *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn );

#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
