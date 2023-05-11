/***********************************************************************
 * @file      circular_buffer.c
 * @version   0.1
 * @brief     Function implementation file.
 *
 * @author    Jasan Preet Singh, Jasan.Singh@Colorado.edu
 * @date      Dec 22, 2021
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 1.5 - Circular Buffer
 * @due        
 *
 * @resources   
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


#include "circular_buffer.h"


// Declare memory for the queue/buffer/fifo, and the write and read pointers
queue_struct_t   my_queue[QUEUE_DEPTH]; // the queue
uint32_t         wptr = 0;              // write pointer
uint32_t         rptr = 0;              // read pointer
// Decide how will you handle the full condition. 
bool flag; // flag to track the full condition


// ---------------------------------------------------------------------
// Private function used only by this .c file. 
// compute next ptr value
// Isolation of functionality: This defines "how" a pointer advances. 
// ---------------------------------------------------------------------
static uint32_t nextPtr(uint32_t ptr) {

  // Student edit:
  // Create this function
  //return (( ptr + 1) % QUEUE_DEPTH);
  if( ptr + 1 == QUEUE_DEPTH)
   return 0;
  else 
   return ptr+1;
  

} // nextPtr()


// ---------------------------------------------------------------------
// Public function
// This function writes an entry to the queue.
// Returns false if successful or true if writing to a full fifo.
// ---------------------------------------------------------------------
bool write_queue (uint16_t charHandle, size_t bufferLength, uint8_t* buffer ) {

  // Student edit:
  // Create this function
  // Decide how you want to handle the "full" condition.

  // Isolation of functionality: 
  //     Create the logic for "when" a pointer advances
  
  bool full, empty;

  //get status
  get_queue_status (&wptr, &rptr, &full, &empty);

  if(full)
  {
   
   return true;
  }
  else 
  {
    //write to the queue
    my_queue[wptr].charHandle   = charHandle;
    my_queue[wptr].bufferLength = bufferLength;

    for(size_t i=0; i<bufferLength; i++)
      {
        my_queue[wptr].buffer[i] = buffer[i];
      }

    //wptr neads to be updated
    wptr = nextPtr(wptr);
    
    if( wptr == rptr)
      flag = true;
    
    return false;
  }  

} // write_queue() 


// ---------------------------------------------------------------------
// Public function
// This function reads an entry from the queue.
// Returns false if successful or true if reading from an empty fifo. 
// ---------------------------------------------------------------------
bool read_queue (uint16_t* charHandle, size_t* bufferLength, uint8_t* buffer) {

  // Student edit:
  // Create this function
  
  // Isolation of functionality: 
  //     Create the logic for "when" a pointer advances
  
  bool full, empty;

  //get status
  get_queue_status (&wptr, &rptr, &full, &empty);

  if(empty)
   return true;
  else
  {
   *charHandle = my_queue[rptr].charHandle;
   *bufferLength = my_queue[rptr].bufferLength;
    for(int i=0; i< *bufferLength; i++)
      {
        buffer[i] = my_queue[rptr].buffer[i];
      }


  //updating the flag if the buffer was full while reading
   if( flag )
    flag = false;
  
   rptr = nextPtr(rptr);
   return false;
  }
  
} // read_queue() 



// ---------------------------------------------------------------------
// Public function
// This function returns the wptr, rptr, full and empty values.
// ---------------------------------------------------------------------
void get_queue_status (uint32_t *_wptr, uint32_t *_rptr, bool *_full, bool *_empty) {

  // Student edit:
  // Create this function
  //uint32_t length = get_queue_depth();

  if( (wptr == rptr) && flag )
  { *_full = true; *_empty = false; }
  else
   *_full = false;
  
  if( (wptr == rptr) && !flag)
  { *_empty = true; *_full = false; }
  else 
    *_empty = false;

  *_wptr = wptr;
  *_rptr = rptr;



} // get_queue_status() 



// ---------------------------------------------------------------------
// Public function
// Function that computes the number of entries in the queue
// ---------------------------------------------------------------------
uint32_t get_queue_depth() {

  // Student edit:
  // Create this function
  if( (wptr == rptr) && !flag )
   return 0;
  else if( (wptr == rptr) && flag )
  {
    return QUEUE_DEPTH;
  }

  
  if( (int)(wptr - rptr) < 0 )
  return (QUEUE_DEPTH + (wptr - rptr));
  
  return (wptr - rptr + 1);
  

} // get_queue_depth() 








