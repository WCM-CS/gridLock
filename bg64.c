#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bg64.h"
#include <time.h>



//GAME STATE: Memory Layout
Arena 
GameArena_Allocation(usize size) 
{
    // malloc 1MB aligned to 64 bytes
    u8 *raw_memory = (u8 *)aligned_alloc(64, size);

    if (raw_memory == NULL) {
        printf("ALLOCATION FAILED\n");
        exit(1);
    }

    // Zero the entire arena
    if (raw_memory) memset(raw_memory, 0, size);

    // Initialize the game arena with the mem block
    Arena game_arena = {
        .base = raw_memory, 
        .size = size, 
        .offset = 0 
    };

    return game_arena;
}


GameState*
GameState_Allocation(Arena *arena)
{
    // Ensure struct alignment (safety buffer, not for bit mask modulus but rather memory alignment)
    arena->offset = (arena->offset + 63) & ~63;

    // slice game state from the memory address in the arena
    GameState *state = (GameState *)(arena->base + arena->offset);

    // Increment the offset, sealing that data in the bump
    arena->offset += sizeof(GameState);

    return state;
}

void
GameState_Initialization(GameState *state)
{

    usize items = load_state("save.bin", state);
    if (items == 1) return;
    if (items == 0) {
        // Wipe Memory
        memset(state, 0, sizeof(GameState));
 
        // Set Metadata
        state->utility.magic = 0x474C4B21; // "GLK!" in hex (for GridLock)
        state->utility.version = 1;
        state->utility.rng_seed = (u64)time(NULL);
        if (state->utility.rng_seed == 0) state->utility.rng_seed = 0xFEED;

        // Setup Palette
        state->utility.palette[0] = (Color){ 0, 0, 0, 0 };
        state->utility.palette[1] = (Color){ 255, 80, 80, 255 };
        state->utility.palette[2] = (Color){ 80, 255, 80, 255 };
        state->utility.palette[3] = (Color){ 80, 80, 255, 255 };

        // Defaults for session
        state->session.dragging_slot_index = 255;
        state->utility.current_screen = 0; // default main screen

        // Fill the queue
        fill_queue(state);

        // Set the deck
        u8 count = ring_buffer_consume_batch(state, state->session.deck_shape_color_bits, 3);
        for (u8 i = 0; i < count; i++) {
            state->session.is_active[i] = true;
        }
    }
}






// GAME STATE: FILE IO
usize 
save_state(const char* file, GameState* state)
{
    FILE* f = fopen(file, "wb");
    if (!f) {
        printf("Failed to open save state file");
        return 0;
    }

    usize items = fwrite(state, sizeof(GameState), 1, f);
    fclose(f);

    return (items == 1);
}

usize 
load_state(const char* file, GameState* state)
{
    FILE* f = fopen(file, "rb");

    if (!f) {
        printf("Failed to read state file bytes");
        return 0;
    }

    usize items = fread(state, sizeof(GameState), 1, f);
    fclose(f);

    if (items < 1) {
        printf("Read files bytes, contents was empty. Potential corruption of file");
    }

    return items;
}




// QUEUE

bool 
ring_buffer_produce(GameState *state, u8 data)
{
    if (state->utility.ring_buffer_counter >= 64) return false; 

    state->ring_buffer[state->utility.ring_buffer_write_index++ & 63] = data;
    state->utility.ring_buffer_counter++;

    return true;
} // ++ increments after indexing with the starting value


u8 
ring_buffer_consume(GameState *state)
{
    if (state->utility.ring_buffer_counter == 0) return 0;

    u8 data = state->ring_buffer[state->utility.ring_buffer_read_index++ & 63];
    state->utility.ring_buffer_counter--;
    
    return data;
}


u8 
ring_buffer_consume_batch(GameState *state, u8 *batch, u8 max_batch_size)
{
    u8 available_bytes = state->utility.ring_buffer_counter;
    if (available_bytes == 0 || max_batch_size == 0) return 0;

    // Take smallest variable as upper bounds to consume
    u8 consume_up_to = (available_bytes < max_batch_size) ? available_bytes : max_batch_size;

    for (u8 i = 0; i < consume_up_to; i++) {
        batch[i] = state->ring_buffer[state->utility.ring_buffer_read_index++ & 63];
    }

    state->utility.ring_buffer_counter -= consume_up_to;

    return consume_up_to;
}  // max batch size to consume is 64


u8 
ring_buffer_data_available(GameState *state) 
{ 
    return state->utility.ring_buffer_counter;
}


u64 
xorshift(u64 *seed)
{
    u64 x = *seed;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    return *seed = x;
}


u8 
generate_composite_byte(u64 *seed)
{
    // Generate shape and color as per the arrays allocations

    u64 r = xorshift(seed);

    u8 shape = (u8)(r % SHAPE_OPTIONS) + 1; // 10 color opptions 1-10 index
    u8 color = (u8)((r >> 32) % COLOR_OPTIONS) + 1; // 8 valid color options 1-8

    // 0x0F cleans the bytes ensuring 4 bits only
    // composite_byte: [1000 | 1000](example bits) or [high | low] or [shape | color]
    u8 high = (shape & 0x0F) << 4; // left bit shift by 4 to ensure no overlaps in the byte
    u8 low = color & 0x0F; 
 
    // return merged byte storing both a random shape and color
    return (high | low);
}


void 
fill_queue(GameState *state)
{
    // max size - current size = amount needed to refill
    u8 buffer_occupancy = ring_buffer_data_available(state);

    if (buffer_occupancy >= 64) return;

    u8 slots_to_fill = 64 - buffer_occupancy;

    for (u8 i = 0; i < slots_to_fill; i++) {
        u8 composite_byte = generate_composite_byte(&state->utility.rng_seed);

        if (!ring_buffer_produce(state, composite_byte)) {
            TraceLog(LOG_WARNING, "RingBuffer overflow at index %d", state->utility.ring_buffer_write_index);
            break;
        }
    }
}
