#include "bg64_state.h"


//void ring_buffer_initialization(ring_buffer *ring_buffer);
u8 ring_buffer_data_available(ring_buffer *ring_buffer);
bool ring_buffer_produce(ring_buffer *ring_buffer, u8 data);
u8 ring_buffer_consume(ring_buffer *ring_buffer);
u8 ring_buffer_consume_batch(ring_buffer *ring_buffer, u8 *batch, u8 max_batch_size); // pass in a [u8; 256] 

u8 generate_composite_byte();

