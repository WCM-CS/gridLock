#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bg64.h"
#include <time.h>
#include <assert.h>
#include <math.h>

// GAME STATE: Memory Layout
Arena GameArena_Allocation(usize size)
{
    // malloc 1MB aligned to 64 bytes
    u8 *raw_memory = (u8 *)aligned_alloc(64, size);

    if (raw_memory == NULL)
    {
        printf("ALLOCATION FAILED\n");
        exit(1);
    }

    // Zero the entire arena
    if (raw_memory)memset(raw_memory, 0, size);

    // Initialize the game arena with the mem block
    Arena game_arena = {
        .base = raw_memory,
        .size = size,
        .offset = 0};

    return game_arena;
}

GameState *GameState_Allocation(Arena *arena)
{
    // Ensure struct alignment (safety buffer, not for bit mask modulus but rather memory alignment)
    arena->offset = (arena->offset + 63) & ~63;

    // slice game state from the memory address in the arena
    GameState *state = (GameState *)(arena->base + arena->offset);

    // Increment the offset, sealing that data in the bump
    arena->offset += sizeof(GameState);

    return state;
}

void GameState_Initialization(GameState *state)
{

    usize items = load_state("save.bin", state);

    if (items == 1)
    {
        // Fill the queue; since queue may or may not need to be filled here
        fill_queue(state);

        printf("Loaded persistent game state.\n");
    }

    if (items == 0)
    {
        printf("Initializing game state for new user.\n");

        // Wipe Memory
        memset(state, 0, sizeof(GameState));

        // Set Metadata
        state->utility.magic = 0x474C4B21; // "GLK!"
        state->utility.version = 1;
        state->utility.rng_seed = (u64)time(NULL);
        if (state->utility.rng_seed == 0)
            state->utility.rng_seed = 0xFEED;

        // Setup Palette
        state->utility.palette[0] = BLANK; // Empty
        state->utility.palette[1] = RED;   //(Color){ 255, 80, 80, 255 }; // Red
        state->utility.palette[2] = GREEN; //(Color){ 80, 255, 80, 255 }; // Green
        state->utility.palette[3] = BLUE;  //(Color){ 80, 80, 255, 255 }; // Blue

        // Defaults for session
        state->session.dragging_slot_index = 255;
        state->utility.current_screen = 0; // default main screen

        // Fill the queue
        fill_queue(state);

        // Set the deck
        u8 count = ring_buffer_consume_batch(state, state->session.deck_shape_color_bits, 3);
        for (u8 i = 0; i < count; i++)
        {
            state->session.is_active[i] = false;
        }

        printf("Loaded new game state.");
    }
}

// GAME STATE: FILE IO
usize save_state(const char *file, GameState *state)
{
    FILE *f = fopen(file, "wb");
    if (!f)
    {
        printf("Failed to open save state file");
        return 0;
    }

    usize items = fwrite(state, sizeof(GameState), 1, f);
    fclose(f);

    return (items == 1);
}

usize load_state(const char *file, GameState *state)
{
    FILE *f = fopen(file, "rb");

    if (!f)
    {
        printf("Failed to read state file bytes");
        return 0;
    }

    usize items = fread(state, sizeof(GameState), 1, f);
    fclose(f);

    if (items < 1)
    {
        printf("Read files bytes, contents was empty. Potential corruption of file");
    }

    return items;
}

// QUEUE
bool ring_buffer_produce(GameState *state, u8 data)
{
    if (state->utility.ring_buffer_counter >= 64)
        return false;

    state->ring_buffer[state->utility.ring_buffer_write_index++ & 63] = data;
    state->utility.ring_buffer_counter++;

    return true;
} // ++ increments after indexing with the starting value

u8 ring_buffer_consume(GameState *state)
{
    if (state->utility.ring_buffer_counter == 0)
        return 0;

    u8 data = state->ring_buffer[state->utility.ring_buffer_read_index++ & 63];
    state->utility.ring_buffer_counter--;

    return data;
}

u8 ring_buffer_consume_batch(GameState *state, u8 *batch, u8 max_batch_size)
{
    u8 available_bytes = state->utility.ring_buffer_counter;
    if (available_bytes == 0 || max_batch_size == 0)
        return 0;

    // Take smallest variable as upper bounds to consume
    u8 consume_up_to = (available_bytes < max_batch_size) ? available_bytes : max_batch_size;

    for (u8 i = 0; i < consume_up_to; i++)
    {
        batch[i] = state->ring_buffer[state->utility.ring_buffer_read_index++ & 63];
    }

    state->utility.ring_buffer_counter -= consume_up_to;

    return consume_up_to;
} // max batch size to consume is 64

u8 ring_buffer_data_available(GameState *state)
{
    return state->utility.ring_buffer_counter;
}

u64 xorshift(u64 *seed)
{
    assert(seed != 0);

    u64 x = *seed;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    return *seed = x;
}

u8 generate_composite_byte(u64 *seed)
{
    // Generate shape and color as per the arrays allocations

    u64 r = xorshift(seed);

    u8 shape = (u8)(r % SHAPE_OPTIONS) + 1;         // 10 color opptions 1-10 index
    u8 color = (u8)((r >> 32) % COLOR_OPTIONS) + 1; // 8 valid color options 1-8

    // 0x0F cleans the bytes ensuring 4 bits only
    // composite_byte: [1000 | 1000](example bits) or [high | low] or [shape | color]
    u8 high = (shape & 0x0F) << 4; // left bit shift by 4 to ensure no overlaps in the byte
    u8 low = color & 0x0F;

    // return merged byte storing both a random shape and color
    return (high | low);
}

void fill_queue(GameState *state)
{
    // max size - current size = amount needed to refill
    u8 buffer_occupancy = ring_buffer_data_available(state);

    if (buffer_occupancy >= 64)
        return;

    u8 slots_to_fill = 64 - buffer_occupancy;

    for (u8 i = 0; i < slots_to_fill; i++)
    {
        u8 composite_byte = generate_composite_byte(&state->utility.rng_seed);

        if (!ring_buffer_produce(state, composite_byte))
        {
            TraceLog(LOG_WARNING, "RingBuffer overflow at index %d", state->utility.ring_buffer_write_index);
            break;
        }
    }
}

// Rendering
void RenderCenteredText(const char *text, u32 y, u32 font_size, Color color, u32 virtual_width)
{
    u32 text_width = MeasureText(text, font_size);
    u32 text_pos = (virtual_width / 2) - (text_width / 2);
    DrawText(text, text_pos, y, font_size, color);
}


void UpdateMenus(GameState *state, Vector2 virtual_mouse)
{
    // virtual width 360
    if (CheckCollisionPointRec(virtual_mouse, PLAY_BUTTON) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        state->utility.current_screen = 1; 
    }
}

void UpdateGameLogic(GameState *state, Vector2 virtual_mouse, u32 offsetX, u32 offsetY, u32 cellSize)
{
    if (!state->session.is_dragging) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (u8 i = 0; i < 3; i++) {
                if (state->session.is_active[i]) continue;


                Rectangle slot_hit = { DECK_SLOTS[i].x - 20, DECK_SLOTS[i].y - 20, 40, 40 };
                if (CheckCollisionPointRec(virtual_mouse, slot_hit)) {
                    state->session.is_dragging = true;
                    state->session.dragging_slot_index = i;

                    state->session.drag_offset.x = DECK_SLOTS[i].x - virtual_mouse.x;
                    state->session.drag_offset.y = DECK_SLOTS[i].y - virtual_mouse.y;

                    // ensures first render doesnt glitch
                    state->session.drag_pos.x = virtual_mouse.x + state->session.drag_offset.x;
                    state->session.drag_pos.y = virtual_mouse.y + state->session.drag_offset.y;

                    return; 
                }
            }
        }

        return;
    }

    // 2. Drag Update (Continuously update position while button is held)
    state->session.drag_pos.x = virtual_mouse.x + state->session.drag_offset.x;
    state->session.drag_pos.y = virtual_mouse.y + state->session.drag_offset.y;


    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        int gx = (int)roundf((state->session.drag_pos.x - (cellSize / 2.0f) - offsetX) / (float)cellSize);
        int gy = (int)roundf((state->session.drag_pos.y - (cellSize / 2.0f) - offsetY) / (float)cellSize);  
        //int gx = (int)floorf((state->session.drag_pos.x - offsetX) / (float)cellSize);
        //int gy = (int)floorf((state->session.drag_pos.y - offsetY) / (float)cellSize);

        if (gx >= 0 && gx < 8 && gy >= 0 && gy < 8) {
            u64 mask = 0;
            if (TryPlace(state, state->session.dragging_slot_index, gx, gy, &mask)) {
                // Success: Apply to bitboard and colors
                state->grid.game_grid |= mask;
                BakeColorsIntoGrid(state, mask, state->session.dragging_slot_index);

                ClearLinesAndColors(state);
                
                state->session.is_active[state->session.dragging_slot_index] = true;
                //state->session.current_score += 10;

                // Refill Deck Check
                if (state->session.is_active[0] && state->session.is_active[1] && state->session.is_active[2]) {
                    ring_buffer_consume_batch(state, state->session.deck_shape_color_bits, 3);
                    state->session.is_active[0] = state->session.is_active[1] = state->session.is_active[2] = false;
                }
            }
        
        }
        
        // Reset dragging state regardless of success
        state->session.is_dragging = false;
        state->session.dragging_slot_index = 0; 
        state->session.drag_offset = (Vector2){ 0, 0 };
        state->session.drag_pos = (Vector2){ 0, 0 };
    }
}





void BakeColorsIntoGrid(GameState *state, u64 mask, u8 slot_index) 
{
    u8 color = GET_COLOR(state->session.deck_shape_color_bits[slot_index]);

    for (int i = 0; i < 64; i++) {
        // We check if the bit is set using the "Big Endian" (63-i) style
        if ((mask >> (63 - i)) & 1) {
            u8 byte_idx = i / 2;
            if (i % 2 == 0) {
                state->grid.grid_color[byte_idx] = (state->grid.grid_color[byte_idx] & 0x0F) | (color << 4);
            } else {
                state->grid.grid_color[byte_idx] = (state->grid.grid_color[byte_idx] & 0xF0) | (color & 0x0F);
            }
        }
    }
}

void ClearLinesAndColors(GameState *state) 
{
    u64 rows_to_clear = 0;
    u64 cols_to_clear = 0;
    u32 lines_cleared_count = 0;

    for (int i = 0; i < 8; i++) {
        if ((state->grid.game_grid & ROW_MASKS[i]) == ROW_MASKS[i]) {
            rows_to_clear |= ROW_MASKS[i];
            lines_cleared_count++; // Found a row!
        }
        if ((state->grid.game_grid & COL_MASKS[i]) == COL_MASKS[i]) {
            cols_to_clear |= COL_MASKS[i];
            lines_cleared_count++; // Found a column!
        }
    }

    u64 total_clear_mask = rows_to_clear | cols_to_clear;

    if (total_clear_mask != 0) {
        // THE ERASER: This clears the logical blocks from the bitboard
        state->grid.game_grid &= ~total_clear_mask;

        for (int i = 0; i < 64; i++) {
            if ((total_clear_mask >> (63 - i)) & 1) {
                u8 byte_idx = i >> 1;
                if ((i & 1) == 0) state->grid.grid_color[byte_idx] &= 0x0F;
                else state->grid.grid_color[byte_idx] &= 0xF0;
            }
        }

        // 4. Update Score: 10 points per line
        state->session.current_score += (lines_cleared_count * 10);

        // Now clear the colors for those specific bits
        // for (int i = 0; i < 64; i++) {
        //     if ((total_clear_mask >> (63 - i)) & 1) {
        //         u8 byte_idx = i / 2;
        //         if (i % 2 == 0) state->grid.grid_color[byte_idx] &= 0x0F; // Clear high nibble
        //         else state->grid.grid_color[byte_idx] &= 0xF0;           // Clear low nibble
        //     }
        // }

        // state->session.current_score += 10;
    }
}






bool TryPlace(GameState *state, u8 slot_idx, int gx, int gy, u64 *out_mask) 
{
    // 1. HARD BOUNDARY: If the anchor itself is totally off-screen
    if (gx < 0 || gy < 0 || gx >= 8 || gy >= 8) return false;

    u8 composite = state->session.deck_shape_color_bits[slot_idx];
    u64 shape_mask = SHAPE_LIBRARY[GET_SHAPE(composite)];

    // 2. SPILLOVER CHECK: Detect if bits wrap from right-edge to left-edge
    for (int i = 0; i < 4; i++) {
        // Extract 8 bits for the current row of the 4x4 shape
        u8 row = (u8)((shape_mask >> (56 - (i * 8))) & 0xFF);
        if (row > 0) {
            // Find how many bits wide this specific row is
            // __builtin_clz works on 32-bit ints, so we adjust for our 8-bit row
            int row_width = 8 - __builtin_ctz(row); 
            
            // If anchor gx + the width of this row > 8, it's a spillover
            if (gx + row_width > 8) return false;
            // If the row exists but the anchor gy pushes it off the bottom
            if (gy + i > 7) return false;
        }
    }

    // 3. BITWISE SHIFT: Move shape to the grid coordinates
    // gy * 8 moves it down rows, gx moves it across columns
    u64 shifted = shape_mask >> (gy * 8 + gx);

    // 4. COLLISION CHECK: Is any bit already occupied?
    if (state->grid.game_grid & shifted) return false;

    // Success! Pass the mask back to be 'baked' into the grid
    *out_mask = shifted;

    return true;
}




void RenderMainScreen(GameState *state, u32 virtual_width, Vector2 virtualMouse)
{
    // render main screen
    RenderCenteredText("giridLock Menus", 200, 40, LIGHTGRAY, virtual_width);
    RenderCenteredText(TextFormat("High Score: %i", state->session.high_score), 260, 25, LIGHTGRAY, virtual_width);

    Rectangle playBtn = { (virtual_width / 2.0f) - 80, 400, 160, 50 };
    Color btnColor = CheckCollisionPointRec(virtualMouse, playBtn) ? GRAY : BLACK;

    DrawRectangleRec(playBtn, btnColor);
    RenderCenteredText("PLAY", playBtn.y + 15, 20, RAYWHITE, virtual_width);

}


void RenderGameScreen(GameState *state, u32 offsetX, u32 offsetY, u32 cellSize, i32 virtual_width)
{
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // Calculate position for each cell
            u32 bit_index = y * 8 + x;
            Rectangle cell = { 
                (float)offsetX + (x * cellSize), 
                (float)offsetY + (y * cellSize), 
                (float)cellSize, 
                (float)cellSize 
            };

            // Option A: Draw faint lines
            DrawRectangleLinesEx(cell, 1.0f, Fade(BLACK, 0.5f));

            if ((state->grid.game_grid >> (63 - bit_index)) & 1) {
                u8 byte_idx = bit_index / 2;
                u8 color_idx = (bit_index % 2 == 0) ? (state->grid.grid_color[byte_idx] >> 4) & 0x0F : state->grid.grid_color[byte_idx] & 0x0F;

                DrawRectangleRec(cell, state->utility.palette[color_idx]);
                DrawRectangleLinesEx(cell, 1.0f, ColorAlpha(BLACK, 0.2f));
            }
        }
    }

    RenderCenteredText(TextFormat("Score: %i", state->session.current_score), 400, 25, RAYWHITE, virtual_width);


    for (u8 i = 0; i < 3; i++) {
        if (state->session.is_active[i]) continue;

        u8 composite = state->session.deck_shape_color_bits[i];
        u64 shape_mask = SHAPE_LIBRARY[GET_SHAPE(composite)];
        Color c = state->utility.palette[GET_COLOR(composite)];

        Vector2 draw_pos = (state->session.is_dragging && state->session.dragging_slot_index == i) 
                            ? state->session.drag_pos 
                            : DECK_SLOTS[i];

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if ((shape_mask >> (63 - (y * 8 + x))) & 1) {
                    float bx = draw_pos.x + (x * cellSize) - (cellSize / 2.0f);
                    float by = draw_pos.y + (y * cellSize) - (cellSize / 2.0f);

                    DrawRectangle(bx, by, cellSize, cellSize, c);
                    DrawRectangleLinesEx((Rectangle){bx, by, (float)cellSize, (float)cellSize}, 2.0f, ColorAlpha(BLACK, 0.3f));
                }
            }
        }
    }

}