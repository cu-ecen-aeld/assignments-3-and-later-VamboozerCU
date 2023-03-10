/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */



//#ifdef __KERNEL__
/*
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/mutex.h>
#include <linux/slab.h>
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h> // replaces stddef.h, stdint.h, & stdbool.h
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include "aesdchar.h"
//#include "aesd-circular-buffer.h"
#include <linux/mutex.h>
#include <linux/gfp.h>
//#include <linux/uaccess.h>

//#else
/*
#include <stddef.h> // size_t
#include <stdint.h> // uintx_t
#include <stdbool.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/mutex.h>
#include <linux/slab.h>
*/
//#endif

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Marshall Folkman");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

void aesd_add_entryK(struct aesd_circular_bufferK *buffer, const struct aesd_buffer_entry *add_entry) {
    // Allocate memory for the new entry
    struct aesd_buffer_entry *new_entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
    if (!new_entry) {
        // Handle allocation failure
        return;
    }
    // Copy the new entry data into the allocated buffer
    memcpy(new_entry, add_entry, sizeof(struct aesd_buffer_entry));

    // Check if the buffer is full
    if (buffer->full) {
        // Free the oldest entry
        kfree(buffer->entry[buffer->out_offs]->buffptr);
        kfree(buffer->entry[buffer->out_offs]);

        // Set the buffer entry pointer to the new allocated entry
        buffer->entry[buffer->out_offs] = new_entry;

        // Increment out_offs to index the new oldest entry
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    } 
    else {
        // Add the new entry to the buffer at the current in_offs index
        buffer->entry[buffer->in_offs] = new_entry;
        // Increment in_offs to index the next index for the next entry
        buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        // If in_offs equals out_offs, the buffer is now full
        buffer->full = (buffer->in_offs == buffer->out_offs);
    }
}

struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fposK(struct aesd_circular_bufferK *buffer,
            size_t char_offset, loff_t *entry_offset_byte_rtn )
{
    size_t current_offset = 0;
    struct aesd_buffer_entry *entry;
    size_t i, j;

    for(i = buffer->out_offs, j=0; 
        ((i != buffer->in_offs) || buffer->full) && (j < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED); 
        i = (i + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED, j++)
    {
        entry = (buffer)->entry[i];
        if((current_offset + entry->size) > char_offset){
            *entry_offset_byte_rtn = char_offset - current_offset;
            return entry;
        }
        current_offset += entry->size;
    }
    return NULL;
}

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *local_dev;

    PDEBUG("opening...");

    local_dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = local_dev;

    //aesd_circular_buffer_init(&aesd_device.circular_buffer); already done in init

    PDEBUG("open done.");
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("releasing...");
    
    filp->private_data = NULL;

    PDEBUG("releasing done.");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *localDev = filp->private_data;
    struct aesd_buffer_entry *entry;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    //mutex_lock_interruptible(&aesd_device.my_mutex);
    retval = (ssize_t)mutex_lock_interruptible(&aesd_device.my_mutex);
    if (retval)
        return retval;

    entry = aesd_circular_buffer_find_entry_offset_for_fposK(&localDev->circular_buffer, count, f_pos);
    retval = *f_pos;

    mutex_unlock(&aesd_device.my_mutex);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *localDev = filp->private_data;
    struct aesd_buffer_entry entryCurrent;
    char *localBuffer = kmalloc(count, GFP_KERNEL);
    bool bNewlineCharFound = false;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    //mutex_lock_interruptible(&aesd_device.my_mutex);
    retval = (ssize_t)mutex_lock_interruptible(&aesd_device.my_mutex);
    if (retval)
        return retval;

    // Steps to append new write data to current write buffer
    aesd_device.currentWriteBuffer = krealloc(aesd_device.currentWriteBuffer, 
                                    (aesd_device.currentWriteBuffer_Size + count), 
                                    GFP_KERNEL);
    if(copy_from_user(localBuffer, buf, count)){
        PDEBUG("ERROR: Failed to copy_from_user within aesd_write.");
        retval = -EFAULT;
        goto exit_cleanup;
    }
 
    // copy new data from localBuffer to the end of aesd_device.currentWriteBuffer
    memcpy(aesd_device.currentWriteBuffer + aesd_device.currentWriteBuffer_Size, localBuffer, count);

    aesd_device.currentWriteBuffer_Size += count;
    retval = count;

    // Check for end of packet (check for '\n'). If we're at the
    // end of a new packet, add packet as new entry into circular
    // buffer and reset the current write buffer in preparation
    // for the next packet.
    for (int i = 0; i < count; i++){
        if (localBuffer[i] == '\n'){
            bNewlineCharFound = true; // newline character found
            break;
        }
    }

    if(bNewlineCharFound){
        // Add current write buffer as new entry into circular buffer
        entryCurrent.buffptr = aesd_device.currentWriteBuffer;
        entryCurrent.size = aesd_device.currentWriteBuffer_Size;
        aesd_add_entryK(&localDev->circular_buffer, &entryCurrent);

        // Reset the current write buffer in preparation for the next packet.
        aesd_device.currentWriteBuffer_Size = 0;
        aesd_device.currentWriteBuffer = krealloc(aesd_device.currentWriteBuffer, 1, GFP_KERNEL);
    }

exit_cleanup:
    mutex_unlock(&aesd_device.my_mutex);
    kfree(localBuffer);
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }

    memset(&aesd_device,0,sizeof(struct aesd_dev));
    mutex_init(&aesd_device.my_mutex);
    aesd_device.currentWriteBuffer = kmalloc(sizeof(char), GFP_KERNEL);
    if (!aesd_device.currentWriteBuffer) {
        // handle allocation error
        PDEBUG("ERROR: Failed to allocate memory for currentWriteBuffer.");
        goto exit_cleanup;
    }

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

exit_cleanup:
    aesd_cleanup_module();
    return result;
}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    uint8_t index;
    struct aesd_buffer_entry *entry;

    cdev_del(&aesd_device.cdev);

    AESD_CIRCULAR_BUFFER_FOREACH(entry, aesd_device.circular_buffer, index) {
        kfree(entry->buffptr);
        kfree(entry);
    }

    kfree(&aesd_device.my_mutex);
    kfree(&aesd_device.currentWriteBuffer);

    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
