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

#include <stdio.h>
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
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    if(buffer == NULL)
        return NULL;

    /* find what entry char_offset is present in, and return a pointer to the associated entry */

    size_t currPos      = 0;
    size_t posDif       = 0;
    size_t totalBytes   = 0;
    uint8_t entryCnt    = 0;

/** 
 * 
 * loop through all entries and sum total bytes 
 * 
 **/
    while(buffer->entry[entryCnt].buffptr != NULL)
        totalBytes += buffer->entry[entryCnt++].size;

    printf("\nTotal Bytes: %ld", totalBytes);

/* we dont have that many bytes in the buffer, return NULL */
    if(char_offset > totalBytes)
        return NULL;

/* loop until we find the byte offset we want */
    entryCnt = 0;
    while(currPos != char_offset)
    {
        printf("\nEntry: %d, String: %s, size %ld", entryCnt, buffer->entry[entryCnt].buffptr, buffer->entry[entryCnt].size);
        /* move through each entry until we find what entry the byte we want is in */
        if(((buffer->entry[entryCnt].size - 1) + currPos) < char_offset)
        {
            currPos += buffer->entry[entryCnt].size;
            entryCnt++;
            
        }
        else
        {
            /* determine desired position offset from the entry that contains it */
            posDif = char_offset - currPos;
            currPos = char_offset;
        }
    }

    /* save a pointer to the desired character */
    entry_offset_byte_rtn = &posDif;
    printf("\nchar_offset = %ld, Entry: %d", char_offset, entryCnt);
    printf("\nentry_offset_byte_rnt: %ld, posDif = %ld\n\n", *entry_offset_byte_rtn, posDif);
    buffer->entry[entryCnt].buffptr += posDif;

    return (&(buffer->entry[entryCnt]));
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
/* error out if either buffer or entry are invalid */
    if(buffer == NULL || add_entry == NULL)
    {
        /* do nothing, break*/
        printf("\nbuffer or add_entry was NULL");
    }
    else
    {
    /* wrap input counter if current offset is at the end of the array */
        if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED + 1)
        {    
            printf("\nin_offs = %d, wrapping back to 0", buffer->in_offs);
            buffer->in_offs = 0;
        }

    /* add entry to entry array at the current input offset */
        printf("\nwriting entry %d", buffer->in_offs);
        buffer->entry[buffer->in_offs] = *add_entry;
    /* increment input offset after new entry added */
        buffer->in_offs++;

    /* if in and out are equal after a write, then the buffer is full */
        if(buffer->out_offs == buffer->in_offs)
        {
            printf("\nBuffer Full");
            buffer->full = true;
        }

    /** 
     *  if the buffer was full and we did another write, we overwrote some data 
     *  which means we need to increment the output offset
     **/
        if(buffer->in_offs > buffer->out_offs && buffer->full == true)
        {
            printf("\nmoving out_offs, overwrite occurred");
            buffer->out_offs++;
        }    
    }

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}