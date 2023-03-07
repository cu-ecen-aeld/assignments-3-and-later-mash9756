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
    for(int i = 0; i <= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED-1; i++)
    {
        totalBytes += buffer->entry[i].size;
    }    


/* we dont have that many bytes in the buffer, return NULL */
    if(char_offset >= totalBytes)
    {
        printf("\noffset greater than total bytes, NULL\n");
        return NULL;
    }

    printf("\nTotal Bytes: %ld", totalBytes);
    printf("\nout_offs: %d", buffer->out_offs);
    printf("\nchar_offset: %ld", char_offset);

/* loop until we find the byte offset we want */
    entryCnt = 0; //buffer->out_offs;
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
    *entry_offset_byte_rtn = posDif;
    //printf("\nchar_offset = %ld, Entry: %d", char_offset, entryCnt);
    printf("\nentry_offset_byte_rnt: %ld, posDif = %ld\n\n", *entry_offset_byte_rtn, posDif);

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
        //printf("\nbuffer or add_entry was NULL");
    }
    else
    {
    /* add entry to entry array at the current input offset */
        printf("writing entry %d\n", buffer->in_offs);
        buffer->entry[buffer->in_offs] = *add_entry;
        if(buffer->in_offs == 0)
        {
            printf("\nentry %d: %s", buffer->in_offs, buffer->entry[buffer->in_offs].buffptr);
            printf("\nentry %d: %s", buffer->in_offs+1, buffer->entry[buffer->in_offs+1].buffptr);
            printf("\nentry %d: %s", buffer->in_offs+2, buffer->entry[buffer->in_offs+2].buffptr);
        }
    /* increment input offset after new entry added */
        buffer->in_offs++;

    /* wrap input counter if current offset is at the end of the array */
        if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {    
            printf("\nin_offs = %d, wrapping back to 0\n", buffer->in_offs);
            buffer->in_offs = 0;
        }

    /* if in and out are equal after a write, then the buffer is full */
        if(buffer->out_offs == buffer->in_offs)
        {
            printf("\n\nBuffer Full\n\n");
            buffer->full = true;
        }

    /** 
     *  if the buffer was full and we did another write, we overwrote some data 
     *  which means we need to increment the output offset
     **/
    
        if(buffer->in_offs > buffer->out_offs && buffer->full == true)
        {
            //printf("\nout_offs = %d", buffer->out_offs);
            //printf("\nmoving out_offs, overwrite occurred");
            buffer->out_offs++;
            //printf("\nout_offs = %d\n", buffer->out_offs);
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