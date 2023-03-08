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
 *  @name   dump_buffer
 *  @brief  dumps full circular buffer
 * 
 *  @param  buffer  pointer to circular buffer instance
 * 
 *  @return VOID
*/
void dump_buffer(struct aesd_circular_buffer *buffer)
{
    printf("\nDumping Buffer...");
    for(int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
        printf("\nentry %d Size = %ld, string = %s", i, buffer->entry[i].size, buffer->entry[i].buffptr);
}

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
    if(buffer == NULL || entry_offset_byte_rtn == NULL)
        return NULL;

/* holds start position, calclated based on current out_offs entry */
    size_t startPos = 0;

/* calculate desired raw start offset based on current out_offs (entry #) */
    if(buffer->out_offs == 0)
        startPos = 0;
    else
    {
        for(int i = 0; i < buffer->out_offs; i++)
            startPos += buffer->entry[i].size;
        printf("\nstartPos = %ld", startPos);
    }
    printf("\nchar_offset = %ld", char_offset);

    size_t rawOffs      = 0;
/* holds our current BYTE position as we move through buffer */
    size_t currPos      = 0;
/* holds the desired byte offset from the 0th byte of the entry that contains it */
    size_t posDif       = 0;
    size_t totalBytes   = 0;
    uint8_t entryCnt    = 0;

/* true raw offset position we are looking for (ref is 0th byte of entry 0) */
    rawOffs = startPos + char_offset;

/* loop through all entries and sum total bytes */
    for(int i = 0; i <= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED-1; i++)
        totalBytes += buffer->entry[i].size; 

    printf("\nTotal Bytes: %ld", totalBytes);

/* we dont have that many bytes in the buffer, return NULL */
    if(char_offset >= totalBytes)
    {
        printf("\noffset greater than total bytes, NULL\n\n");
        return NULL;
    }
/* some wacky math to wrap the raw offset byte position if we are reading from non-0 entry */
    else if(char_offset >= totalBytes - startPos)
    {
    /* should give us a negative value, how many bytes back from start we want to read */
        int fromStartPos = char_offset - totalBytes;
    /* gives us the true raw offset of the desired byte location */
        rawOffs = fromStartPos + startPos;
    }

    printf("\nrawOffs = %ld", rawOffs);
    printf("\nout_offs: %d\n\n", buffer->out_offs);

/* loop until we find the byte offset we want */
    entryCnt = 0;
    while(currPos != rawOffs)
    {
    /* move through each entry until we find what entry the byte we want is in */
        if(((buffer->entry[entryCnt].size) + currPos) <= rawOffs)
        {
            currPos += (buffer->entry[entryCnt].size);
            entryCnt++;
        }
        else
        {
        /* determine desired position offset from the entry that contains it */
            posDif = rawOffs - currPos;
            currPos = rawOffs;
        }
    }
/* save a pointer to the desired character */
    *entry_offset_byte_rtn = posDif;

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
    /* if buffer is full, we need to update out_offs as we are about to overwrite */
        if(buffer->full == true)
        {
            printf("\nout_offs = %d", buffer->out_offs);
            printf("\nmoving out_offs, overwrite occurred");
            buffer->out_offs++;
            printf("\nout_offs = %d\n", buffer->out_offs);
        }

    /* add entry to entry array at the current input offset */
        printf("writing entry %d\n", buffer->in_offs);
        buffer->entry[buffer->in_offs] = *add_entry;

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
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}