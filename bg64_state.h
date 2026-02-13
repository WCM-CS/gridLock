#ifndef BG64_STATE_H_
#define BG64_STATE_H_


#include <stdint.h>      // Standard types 
#include <stddef.h>      // usize
#include <stdbool.h>     // Usually 1 byte
#include <stdalign.h>    // struct cache alignment
#include "raylib.h"      // Graphics API


// Data Type Casts       // Memory Size

// Unsigned
typedef uint8_t u8;      // 1 byte
typedef uint16_t u16;    // 2 bytes 
typedef uint32_t u32;    // 4 bytes 
typedef uint64_t u64;    // 8 bytes 
typedef size_t usize;    // 4 or 8 bytes 

// Signed
typedef int8_t i8;       // 1 byte
typedef int16_t i16;     // 2 bytes
typedef int32_t i32;     // 4 bytes
typedef int64_t i64;     // 8 bytes


// Floats
typedef float f32;       // 4 bytes
typedef double f64;      // 8 bytes
typedef long double f80; // 16 bytes

// Loop macro
#define loop for(;;)


// 1MB 
static const usize ARENA_SIZE = 1024 * 1024;

typedef struct {
    u8 *base;      // Pointer to the BLOCK-O-MEM
    usize size;    // Total capacity (1MB)
    usize offset;  // Current offset bump increment
} Arena;



// Step 4: Color Mapping
//     - Given the u8 from the grid state use it to index into the color palette
//     - This maps our u8 representation to a color we can pass to raylib
static const u8 PALETTE_SIZE = 3;
static const Color PALETTE[16] = { // 3
    [0] = { 0, 0, 0, 0 },        // Void
    [1] = { 255, 80, 80, 255 },  // Red
    [2] = { 80, 255, 80, 255 },  // Green
    [3] = { 80, 80, 255, 255 },  // Blue
}; // Grid State id will be used to index into this Array

// These act as a canvas or stelcil we will use
// we will overlap the result u64 over the game grid u64 to see if it fits
// if any bits in the res u64 overlap with the game tate u64 we know it cannot fit 
// Top aligned shapes
static const u8 SHAPE_LIB_SIZE = 10;
static const u64 SHAPE_LIBRARY[16] = { // 10
    [0]  = 0, // Void

    // [X]
    [1]  = 0x8000000000000000,                // 1x1 Dot

    // [X][X]
    [2]  = 0xC000000000000000,                // 2x1 Horizontal

    // [X][X][X]
    [3]  = 0xE000000000000000,                // 3x1 Horizontal

    // [ ][X][ ]
    // [X][X][X]
    [4]  = 0x40E0000000000000,                // T-Shape (3x2)

    // [X][ ]
    // [X][X][X]
    [5]  = 0x80E0000000000000,                // L-Shape (Small)

    // [X][X]
    // [X][X]
    [6]  = 0xC0C0000000000000,                // 2x2 Square

    // [X][ ][ ]
    // [X][ ][ ]
    // [X][X][X]
    [7]  = 0x8080E00000000000,                // L-Shape (3x3)

    // [X][X][X]
    // [X][X][X]
    // [X][X][X]
    [8]  = 0xE0E0E00000000000,                // 3x3 Big Square

    // [X][X][X][X]
    [9]  = 0xF000000000000000,                // 4x1 Horizontal

    // [X][X]
    // [X][X]
    // [X][X]
    [10] = 0xC0C0C00000000000,               // 2x3 Vertical
};


// Step 1: Ring Buffer Queue
//     - We use a ring buffer of bytes, 256 len to store the blocks.
//     - We use the first 4 bits to store the block size, and the last 4 bits to store the color
//     - This gives us 16 options for block sizes and colors
//     - This amortizes randomization function calls by batch queueing new blocks
typedef struct 
{
    // Current bytes in the ring buffer
    volatile u8 counter;     // 1 byte
    volatile u8 write_index; // 1 byte
    volatile u8 read_index;  // 1 byte

    // u8 Byte stores 4 bits: Block shape, 4 bits: Block Color
    // Block shape is only used to enter the grid, the color is stored in state for raylib rendering
    u8 buffer[64];          // 64 bytes, 21 turns before a refill of blocks
    
    u8 _padding[61];          // 61 bytes
} ring_buffer; // 128 bytes, 2 cache lines




typedef struct 
{
    // Determines the occupancy of each spot on the game grid
    // Raylib needs to check this to see if a block will fit into the given slots
    //  Step 2: Game Board
    // - The game baord has to determine occupancy 
    // - We only need a single u64 for this operation to represent the game grid
    u64 game_grid;  // 8 bytes 

    // Maps the game grid index to a color state
    // each u8 stores the color of two blocks
    // Step 3: Represent grid state
    // - The grid has colored blocks so per each index we need to determine the color
    // - We can simply use a array [u8; 64] for this operation
    // - The index from the u6 grid board tells us to idex into the position to get the color
    u8 grid_color[32];  // 32 bytes
    u8 _padding[24];    // 24 bytes 
    
} game_grid; // 64 bytes, 1 cache line




typedef struct 
{
    u8 shape_color_bits; // 1 byte: 4 bits for shape, 4 for color
    bool is_active;      // 1 byte: determines if the block is in the grid or in the deck (active is in the grid)
    Vector2 pos;         // 8 bytes (2 f32): Raylib coordinates for bitboard dragging
    u8 _padding [4];     // 4 bytes ; padding since Vector2 uses f32 the upper bound for multiples is 4 bytes 
    // 2 ghost bytes 
} deck_slot; // 16 bytes  1/4 cache line

typedef struct
{
    u64 current_score;  // 8 bytes 
    u64 high_score;     // 8 bytes
} user_data; // 16 bytes  1/4 cache line


typedef struct 
{
    game_grid grid;     // 64 bytes
    user_data user;     // 16 bytes 

    deck_slot deck[3];  // 48 bytes

    ring_buffer buffer; // 128 bytes 
} __attribute__((aligned(64))) GameState; // 256 bytes, 4 cache lines 



// move whatever we need for data snapshots into the ring buffer since it has extra space
typedef struct {
    u32 magic;     // 4 bytes 
    u64 checksum;  // 8 bytes
} snapshot;




// Game engine
// store 

/*
    Game Engine
    
    Step 1: Ring Buffer Queue
    - We use a ring buffer of bytes, 256 len to store the blocks.
    - We use the first 4 bits to store the block size, and the last 4 bits to store the color
    - This gives us 16 options for block sizes and colors
    - This amortizes randomization function calls by batch queueing new blocks


    Step 2: Game Board
    - The game baord has to determine occupancy 
    - We only need a single u64 for this operation to represent the game grid

    Step 3: Represent grid state
    - The grid has colored blocks so per each index we need to determine the color
    - We can simply use a array [u8; 64] for this operation
    - The index from the u6 grid board tells us to idex into the position to get the color


    Step 4: Color Mapping
    - Given the u8 from the grid state use it to index into the color palette
    - This maps our u8 representation to a color we can pass to raylib


    Step 5: User Data
    - We need to be able to repesent user data in memory;
    - Storing the current score of a runnign game or the users best score.


    GameState is now a power of 2 (2^8=256), 
*/




Arena GameArena_Initialization(usize size);
GameState* GameState_Initialization(Arena *arena);


usize save_state(const char* file, GameState* state);
usize load_state(const char* file, GameState* state);




#include "bg64_queue.h"

#endif /* RING_BUFFER_H_ */