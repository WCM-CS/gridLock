
#include <stdlib.h>
#include "bg64_state.h"
#include "bg64_queue.h"

// Ring Buffer (not needed since we allcoate game state (whihc included ring buffer) inside the game arena and call memset 0)
// void ring_buffer_initialization(ring_buffer *ring_buffer)
// {
//     ring_buffer->read_index = 0;
//     ring_buffer->write_index = 0;
//     ring_buffer->counter = 0;
// }

u8 generate_composite_byte()
{
    // Generate shape and color as per the arrays allocations
    u8 shape = (rand() % SHAPE_LIB_SIZE) + 1; // 0 index is not a valid option 
    u8 color = (rand() % PALETTE_SIZE) + 1;

    // 0x0F cleans the bytes ensuring 4 bits only
    // composite_byte: [1000 | 1000](example bits) or [high | low] or [shape | color]
    u8 high = (shape & 0x0F) << 4; // left bit shift by 4 to ensure no overlaps in the byte
    u8 low = color & 0x0F; 
 
    // return merged byte storing both a random shape and color
    return (high | low);
}

bool ring_buffer_produce(ring_buffer *ring_buffer, u8 data)
{
    if (ring_buffer->counter >= 64) return false; 

    ring_buffer->buffer[ring_buffer->write_index++ & 63] = data;
    ring_buffer->counter++;

    return true;
} // ++ increments after indexing with the starting value


u8 ring_buffer_consume(ring_buffer *ring_buffer)
{
    if (ring_buffer->counter == 0) return 0;

    u8 data = ring_buffer->buffer[ring_buffer->read_index++ & 63];
    ring_buffer->counter--;
    
    return data;
}


u8 ring_buffer_consume_batch(ring_buffer *ring_buffer, u8 *batch, u8 max_batch_size)
{
    u8 available_bytes = ring_buffer->counter;
    if (available_bytes == 0 || max_batch_size == 0) return 0;

    // Take smallest variable as upper bounds to consume
    u8 consume_up_to = (available_bytes < max_batch_size) ? available_bytes : max_batch_size;

    u8 idx = 0;
    loop {
        batch[idx++] = ring_buffer->buffer[ring_buffer->read_index++ & 63];

        if (idx == consume_up_to) {
            break;
        }
    }

    ring_buffer->counter -= consume_up_to;

    return consume_up_to;
}  // max batch size to consume is 64


u8 ring_buffer_data_available(ring_buffer *ring_buffer) 
{
    return ring_buffer->counter;
}
