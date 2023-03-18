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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Mark Sherman"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    /**
     * TODO: handle open
     * 
     * filp = file pointer, set filp->private_data with aesd_dev device struct
     * use inode->i_cdev with container_of to locate within aesd_dev structure
     */

    struct aesd_dev *dev;
    
    PDEBUG("open");

    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     * 
     * Is this right???
     * Going off W6 Device Driver File Ops, slide 14
     *  "deallocate anything open allocated in filp->private_data"
     */
    filp->private_data = NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    /**
     * TODO: handle read
     * 
     * use filp->private_data to get aesd_dev
     * buff - buffer to fill with data from user space
     *      use copy_to_user
     * count - max number of writes to buffer
     *      may want or need to write less than this value
     * f_pos - pointer to read offset
     *      specific byte in linear content, referenced by char_offset
     *      update to next offset to be read based on number of bytes returned
     * 
     * return == count, requested number of bytes was transferred
     * 0 < return < count, only portion of bytes transferred
     * return == 0, end of file, no data read into buf
     * return < 0, error occurred
     */
    
    size_t unread_cnt = 0;
    size_t read_entry_offset_rtn = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *read_entry;
    ssize_t retval = 0;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    
/* parameter error handling */
    if(filp == NULL || buf == NULL || f_pos == NULL)
    {
        retval = -EFAULT;
        goto exit;
    }

/* obtain mutex, exit on failure */
    if(mutex_lock_interruptible(&dev->mutex) != 0)
    {
        retval = -ERESTARTSYS;
        goto exit;
    }

/* end goal is to find the entry and copy to user space */
    read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &read_entry_offset_rtn);
    if(read_entry == NULL)
    {
        retval = -EFAULT;
        goto exit;
    }
/* check if count would extend past last byte of read entry when starting at desired offset */
    if((read_entry->size - read_entry_offset_rtn) < count)
        count = read_entry->size - read_entry_offset_rtn;

/* returns 0 on success, >0 is number of bytes not read */
    unread_cnt = copy_to_user(buf, (read_entry->buffptr + read_entry_offset_rtn), count);
    if(unread_cnt < 0)
    {
        retval = unread_cnt;
        goto exit;
    }

/* return total bytes read */
    retval = count - unread_cnt;
/* update read offset position */
    *f_pos += retval;

exit:
    mutex_unlock(&dev->mutex);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    /**
     * TODO: handle write
     * 
     * all write appends to command being written until newline
     * write to the command buffer when newline is received
     * 
     * return == count, success
     * 0 < return < count, partial write, retry command
     * return == 0, nothing written, retry command
     * return < 0, error occurred
     * 
     */

    size_t i = 0;
    bool cmd_end_flag = false;
    size_t cmd_len = 0;
    char *input_buffer = NULL;
    size_t unwritten_cnt = 0;
    const char *overwrite = NULL;
    struct aesd_dev *dev = filp->private_data;  /* get pointer to our char device */
    ssize_t retval = 0;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
/* parameter error handling */
    if(filp == NULL || buf == NULL || f_pos == NULL)
    {
        retval = -EFAULT;
        goto exit;
    }

/* obtain mutex, exit on failure */
    if(mutex_lock_interruptible(&dev->mutex) != 0)
    {
        retval = -ERESTARTSYS;
        goto exit;
    }

    input_buffer = (char *)kmalloc(count, GFP_KERNEL);
    if(input_buffer == NULL)
    {
        retval = -ENOMEM;
        goto exit;
    }

/* returns 0 on success, >0 is number of bytes not written */
    unwritten_cnt = copy_from_user(input_buffer, buf, count);
    if(retval < 0)
    {
        retval = -EFAULT;
        goto free_kmem;
    }

    for(i = 0; i < count; i++)
    {
        if(input_buffer[i] == '\n')
        {
            cmd_end_flag = true;
            cmd_len = i + 1;
            break;
        }
    }
/* if we dont find a \n, then our command len needs to be the full size */
    if(cmd_end_flag == false)
        cmd_len = count;
        
/* allocate memory for working entry if it doesnt already exist */
    if(dev->working_entry.size == 0)
        dev->working_entry.buffptr = (char *)kmalloc(cmd_len, GFP_KERNEL);

/* if working entry already contains something, realloc to append more stuff */
    else
        dev->working_entry.buffptr = krealloc(dev->working_entry.buffptr, (cmd_len + dev->working_entry.size), GFP_KERNEL);

    if(dev->working_entry.buffptr == NULL)
    {
        retval = -ENOMEM;
        goto free_kmem;
    }
        
/* intialize allocated working entry buffer */
    memset((void *)dev->working_entry.buffptr, 0, (cmd_len + dev->working_entry.size));

/* copy command contents into entry buffer */
    memcpy((void *)dev->working_entry.buffptr, input_buffer, (cmd_len + dev->working_entry.size));

/* return total bytes written and adjust working entry size accordingly */
    retval = cmd_len - unwritten_cnt;
    dev->working_entry.size += retval;

    if(cmd_end_flag == true)
    {
        overwrite = aesd_circular_buffer_add_entry(&dev->buffer, &dev->working_entry);
    /* will return non-null pointer if old data was overwritten so it can be freed */
        if(overwrite != NULL)
            kfree(overwrite);

        dev->working_entry.buffptr  = NULL;
        dev->working_entry.size     = 0;
    }

free_kmem:
    kfree(input_buffer);

exit:
/* release mutex */
    mutex_unlock(&dev->mutex);

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
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    mutex_init(&aesd_device.mutex);

    result = aesd_setup_cdev(&aesd_device);

    if (result)
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

void aesd_cleanup_module(void)
{
    int count = 0;
    struct aesd_buffer_entry *buffer_element;

    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    AESD_CIRCULAR_BUFFER_FOREACH(buffer_element, &aesd_device.buffer, count)
    {
        if (buffer_element->buffptr != NULL)
        {
            kfree(buffer_element->buffptr);
            buffer_element->size = 0;
        }
    }

    mutex_destroy(&aesd_device.mutex);

    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
