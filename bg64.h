#ifndef BG64_STATE_H_
#define BG64_STATE_H_


#include <stdint.h>      // Standard types 
#include <stddef.h>      // usize
#include <stdbool.h>     // Usually 1 byte
#include <stdalign.h>    // struct cache alignment
#include "raylib.h"      // Graphics API 


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



static const Rectangle PLAY_BUTTON = { (360 / 2.0f) - 80, 400, 160, 50 };


typedef struct 
{
    u8 *base;      // Pointer to the BLOCK-O-MEM
    usize size;    // Total capacity
    usize offset;  // Current offset bump increment
} Arena;


// CACHE LINE 0
// 24 spare bytes
typedef struct 
{
    // Determines the occupancy of each spot on the game grid
    u64 game_grid;  // 8 bytes

    // each u8 stores the color of two 4 bit blocks colors, 64 colors total
    u8 grid_color[32];  // 32 bytes

    u8 _padding[24];    // 24 bytes 
    
} game_grid; // 64 bytes, 1 cache line


// Cache line 1
typedef struct 
{
    // USER SCORES
    u64 current_score;  // 8 bytes ; 8
    u64 high_score;     // 8 bytes ; 16 

    Vector2 drag_pos;         // 8 bytes ; 24 
    Vector2 drag_offset;      // 8 bytes ; 32
    u8 dragging_slot_index;   // 1 byte ; 33
    bool is_dragging;         // 1 byte ; 34

    // DECK BLOCKS
    u8 deck_shape_color_bits[3]; // 3 bytes: 37 ; 4 bits for shape, 4 for color
    bool is_active[3];           // 3 byte: 40 ; determines if the block is in the grid or in the deck (active is in the grids

    u8 _padding [24];     // 24 bytes 
} player_session; // 64 bytes, 1 Cache line



// CACHE LINE 2, 3
// 37 spare bytes in cache line 2
typedef struct 
{
    // Byte generation xor shift seed
    u64 rng_seed; // 8 bytes

    // -- SNAPSHOT METADATA for ENTIRE GAMESTATE--- 
    u64 checksum; // 8 bytes; 16 bytes 
    u32 magic;    // 4 bytes; 20 bytes 
    u32 version;  // 4 bytes; 24 bytes 
    
    // Current bytes in the ring buffer
    volatile u8 ring_buffer_counter;     // 1 byte; 25 bytes 
    volatile u8 ring_buffer_write_index; // 1 byte; 26 bytes
    volatile u8 ring_buffer_read_index;  // 1 byte; 27 bytes 

    // screen state
    u8 current_screen; // 1 byte: 28 bytes

    // -- COLOR PALETTE -- 
    Color palette[9];  // 36 bytes

} utility; // 64 bytes

typedef enum : u8 {
    SCREEN_MENU = 0,
    SCREEN_GAMEPLAY,
    SCREEN_GAMEOVER,
    SCREEN_SETTINGS
} ScreenID;


typedef struct 
{
    game_grid grid;          // 64 bytes
    player_session session;  // 64 bytes 
    utility utility;         // 64 bytes 
    u8 ring_buffer[64];      // 64 bytes 
} __attribute__((aligned(64))) GameState; // 256 bytes, 4 cache lines 



static const Vector2 DECK_SLOTS[3] = {
    { 60.0f,  650.0f },
    { 170.0f, 650.0f },
    { 280.0f, 650.0f }
};


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


static const u64 ROW_MASKS[8] = {
    0xFF00000000000000ULL, 0x00FF000000000000ULL, 0x0000FF0000000000ULL, 0x000000FF00000000ULL,
    0x00000000FF000000ULL, 0x0000000000FF0000ULL, 0x000000000000FF00ULL, 0x00000000000000FFULL
};

static const u64 COL_MASKS[8] = {
    0x8080808080808080ULL, 0x4040404040404040ULL, 0x2020202020202020ULL, 0x1010101010101010ULL,
    0x0808080808080808ULL, 0x0404040404040404ULL, 0x0202020202020202ULL, 0x0101010101010101ULL
};


// GAME STATE
static const usize ARENA_SIZE = 1024 * 1024;

#define SHAPE_OPTIONS 10
#define COLOR_OPTIONS 3

#define GET_SHAPE(composite_byte) ((composite_byte >> 4) & 0x0F)
#define GET_COLOR(composite_byte) (composite_byte & 0x0F)

// Allocations & Inits
Arena GameArena_Allocation(usize size);
GameState* GameState_Allocation(Arena *arena);
void GameState_Initialization(GameState *state);

// File I/O
usize save_state(const char* file, GameState* state);
usize load_state(const char* file, GameState* state);

// Queue (Ring Buffer)
bool ring_buffer_produce(GameState *state, u8 data);
u8 ring_buffer_consume(GameState *state);
u8 ring_buffer_consume_batch(GameState *state, u8 *batch, u8 max_batch_size); // pass in a [u8; 256] 
u8 ring_buffer_data_available(GameState *state);
u64 xorshift(u64 *seed);
u8 generate_composite_byte(u64 *seed);
void fill_queue(GameState *state);



// GameState
bool TryPlace(GameState *state, u8 slot_idx, int gx, int gy, u64 *out_mask);
void UpdateMenus(GameState *state, Vector2 virtual_mouse);\
void UpdateGameLogic(GameState *state, Vector2 virtual_mouse, u32 offsetX, u32 offsetY, u32 cellSize);
void BakeColorsIntoGrid(GameState *state, u64 mask, u8 slot_index);

// Rendering
void RenderCenteredText(const char* text, u32 y, u32 font_size, Color color, u32 virtual_width);
void ClearLinesAndColors(GameState *state);
void RenderMainScreen(GameState *state, u32 virtual_width, Vector2 virtualMouse);
void RenderGameScreen(GameState *state, u32 offsetX, u32 offsetY, u32 cellSize, i32 virtual_width);


#endif /* RING_BUFFER_H_ */