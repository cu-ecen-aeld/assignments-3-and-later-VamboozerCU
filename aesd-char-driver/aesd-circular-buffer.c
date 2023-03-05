/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    size_t current_offset = 0;
    aesd_buffer_entry *entry;

    for (int i = buffer->out_offs, j=0; 
        ((i != buffer->in_offs) || buffer->full) && (j < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED); 
        i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED, j++)
    {
        entry = &buffer->entry[i];
        if((current_offset + entry->size) > char_offset){
            *entry_offset_byte_rtn = char_offset - current_offset;
            return entry;
        }
        current_offset += entry->size;
    }
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(aesd_circular_buffer *buffer, const aesd_buffer_entry *add_entry) {
    // Check if the buffer is full
    if (buffer->full) {
        // Overwrite the oldest entry
        buffer->entry[buffer->out_offs] = *add_entry;
        // Increment out_offs to point to the new oldest entry
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    } 
    else {
        // Add the new entry to the buffer at the current in_offs index
        buffer->entry[buffer->in_offs] = *add_entry;
        // Increment in_offs to point to the next index for the next entry
        buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        // If in_offs equals out_offs, the buffer is now full
        buffer->full = (buffer->in_offs == buffer->out_offs);
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(aesd_circular_buffer *buffer) {
    memset(buffer,0,sizeof(aesd_circular_buffer));
}