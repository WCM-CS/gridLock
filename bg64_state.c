#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bg64_state.h"









Arena GameArena_Initialization(usize size) 
{
    // malloc 1MB aligned to 64 bytes
    u8 *raw_memory = (u8 *)aligned_alloc(64, size);

    // Zero the entire arena
    if (raw_memory) {
        memset(raw_memory, 0, size);
    }

    // Initialize the game arena with the mem block
    Arena game_arena = {
        .base = raw_memory, 
        .size = size, 
        .offset = 0 
    };

    return game_arena;
}


GameState* GameState_Initialization(Arena *arena)
{
    // Ensure struct alignment (safety buffer, not for bit mask modulus but rather memory alignment)
    arena->offset = (arena->offset + 63) & ~63;

    // slice game state len bytes from base offset 
    GameState *state = (GameState *)(arena->base + arena->offset);

    // Increment the offset, sealing that data in the bump
    arena->offset += sizeof(GameState);

    return state;
}




// Game State 
void save_state(const char* file, GameState* state)
{
    FILE* f = fopen(file, "wb");
    if (f) 
    {
        fwrite(state, sizeof(GameState), 1, f);
        fclose(f);
    }
}

usize load_state(const char* file, GameState* state)
{
    FILE* f = fopen(file, "rb");
    usize bytes = 0;
    if (f)
    {
        bytes = fread(state, sizeof(GameState), 1, f);
        if (bytes < 1) {
            printf("Bytes read from file.\n");
        }
        fclose(f);
    } else {
        printf("Failed to read bytes from file");
    }

    return bytes;
}