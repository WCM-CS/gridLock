#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bg64_state.h"
#include <time.h>








Arena 
GameArena_Initialization(usize size) 
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
GameState_Initialization(Arena *arena)
{
    // Ensure struct alignment (safety buffer, not for bit mask modulus but rather memory alignment)
    arena->offset = (arena->offset + 63) & ~63;

    // slice game state from the memory address in the arena
    GameState *state = (GameState *)(arena->base + arena->offset);

    // Set seed for composite byte generation
    state->buffer.rng_seed = (u64)time(NULL);
    if (state->buffer.rng_seed == 0){
        state->buffer.rng_seed = 0xFEED; 
    }

    // Increment the offset, sealing that data in the bump
    arena->offset += sizeof(GameState);

    return state;
}




// Game State 
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

