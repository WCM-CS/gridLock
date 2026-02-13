#include "bg64_state.h"

#define GET_SHAPE(composite_byte) ((composite_byte >> 4) & 0x0F);
#define GET_COLOR(composite_byte) (composite_byte & 0x0F);

bool 
ring_buffer_produce(ring_buffer *ring_buffer, u8 data);

u8 
ring_buffer_consume(ring_buffer *ring_buffer);


u8 
ring_buffer_consume_batch(ring_buffer *ring_buffer, u8 *batch, u8 max_batch_size); // pass in a [u8; 256] 


u8 
ring_buffer_data_available(ring_buffer *ring_buffer);


u64 
xorshift(u64 *seed);

u8 
generate_composite_byte(u64 *seed);

void 
fill_queue(GameState *state);

