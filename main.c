#include <stdio.h>
#include "bg64.h"


int main(void) 
{
    printf("Initializing game state!\n");

    // load in game sate from files, user high sore and entire game state
    // load new seed each session for block gen uniqueness
    Arena game_arena = GameArena_Allocation(ARENA_SIZE);
    GameState *state = GameState_Allocation(&game_arena);
    GameState_Initialization(state);


    const i32 virtual_width = 360;
    const i32 virtual_height = 780;

    const u32 cellSize = 40;
    const u32 gridWidth = 8 * cellSize;
    const u32 offsetX = (virtual_width - gridWidth) / 2;
    const u32 offsetY = 40;


    // Set resolution
    InitWindow(virtual_width, virtual_height, "Blocks");
    SetTargetFPS(60);

    // Make virtual canvas
    RenderTexture2D target = LoadRenderTexture(virtual_width, virtual_height);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR); // bilinear filter does


    while(!WindowShouldClose()) {

        // 1: GETTING USER IO; Input + Coordinates
        Vector2 mouse = GetMousePosition();
        Vector2 virtualMouse = {
            mouse.x * (f32)virtual_width / GetScreenWidth(),
            mouse.y * (f32)virtual_height / GetScreenHeight()
        };


        // 2: Game logic & state updates
        if (state->utility.current_screen == 1) {
            if (!state->session.is_dragging) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    for (int i = 0; i < 3; i++) {
                        // If block is in deck, check collision to start drag
                        if (!state->session.is_active[i]) {
                           
                            Rectangle slotRect = { DECK_SLOTS[i].x - 20, DECK_SLOTS[i].y - 20, 40, 40 };
                            if (CheckCollisionPointRec(virtualMouse, slotRect)) {
                                state->session.is_dragging = true;
                                state->session.dragging_slot_index = i;

                                state->session.drag_pos = virtualMouse;
                                break;
                            }
                        }
                    }
                }
            } else {
                // Dragging is active
                state->session.drag_pos = virtualMouse;

                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    u8 slot_idx = state->session.dragging_slot_index;
                    u8 composite = state->session.deck_shape_color_bits[slot_idx];
                    u64 shape_mask = SHAPE_LIBRARY[GET_SHAPE(composite)];
                    u8 color_idx = GET_COLOR(composite);

    
                    // We use the top-left of the shape to find the grid cell
                    int gridX = (int)((state->session.drag_pos.x - offsetX) / cellSize);
                    int gridY = (int)((state->session.drag_pos.y - offsetY) / cellSize);

                    //boudns and colissions checks
                    bool can_fit = true;
                    // Check every potential bit in the 4x4 shape area
                    for (int y = 0; y < 4; y++) {
                        for (int x = 0; x < 4; x++) {
                            if ((shape_mask >> (63 - (y * 8 + x))) & 1) {
                                int tx = gridX + x;
                                int ty = gridY + y;
                                // Check if bit would fall off the 8x8 grid
                                if (tx < 0 || tx > 7 || ty < 0 || ty > 7) {
                                    can_fit = false; 
                                    break;
                                }
                                // Check if grid is already occupied at that bit
                                if ((state->grid.game_grid >> (63 - (ty * 8 + tx))) & 1) {
                                    can_fit = false;
                                    break;
                                }
                            }
                        }
                    }

                    // exxecute placement insertion
                    if (can_fit) {
                        u64 shifted_mask = shape_mask >> (gridY * 8 + gridX);
                        state->grid.game_grid |= shifted_mask;

                        // Bake colors into the grid nibbles
                        for (int i = 0; i < 64; i++) {
                            if ((shifted_mask >> (63 - i)) & 1) {
                                u8 byte_idx = i / 2;
                                if (i % 2 == 0) state->grid.grid_color[byte_idx] = (state->grid.grid_color[byte_idx] & 0x0F) | (color_idx << 4);
                                else state->grid.grid_color[byte_idx] = (state->grid.grid_color[byte_idx] & 0xF0) | (color_idx & 0x0F);
                            }
                        }

                        state->session.is_active[slot_idx] = true;
                        state->session.current_score += 10; // Placeholder for cell count
                        
                        // TODO: Call your Line Clearing function here!
                    }

                    // cleanup lag
                    state->session.is_dragging = false;

                    // Check for refill
                    if (state->session.is_active[0] && state->session.is_active[1] && state->session.is_active[2]) {
                        ring_buffer_consume_batch(state, state->session.deck_shape_color_bits, 3);
                        state->session.is_active[0] = state->session.is_active[1] = state->session.is_active[2] = false;
                    }
                }
            }
        }







        // Render to 
        BeginTextureMode(target);
            ClearBackground(DARKGRAY);

            // Collect game logic
            switch (state->utility.current_screen) {
                case 0:
                    // render main screen
                    RenderCenteredText("giridLock Menus", 200, 40, LIGHTGRAY, virtual_width);
                    RenderCenteredText(TextFormat("High Score: %i", state->session.high_score), 260, 25, LIGHTGRAY, virtual_width);
                    
                    Rectangle playBtn = { (virtual_width / 2.0f) - 80, 400, 160, 50 };
                    Color btnColor = CheckCollisionPointRec(virtualMouse, playBtn) ? GRAY : BLACK;


                    DrawRectangleRec(playBtn, btnColor);
                    // Center text vertically inside the button (400 + ~15px)
                    RenderCenteredText("PLAY", playBtn.y + 15, 20, RAYWHITE, virtual_width);

                    if (CheckCollisionPointRec(virtualMouse, playBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        state->utility.current_screen = 1; 
                    }
                    break;

                case 1:
                    // render game screen
                    //ClearBackground(DARKGREEN);

                    

                    u32 cellSize = 40;
                    u32 gridWidth = 8 * cellSize;
                    u32 offsetX = (virtual_width - gridWidth) / 2;
                    u32 offsetY = 40;

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
                    
                                // 3. GET THE COLOR: Extract the 4-bit color index
                                u8 byte_idx = bit_index / 2;
                                u8 color_idx = 0;
                                
                                if (bit_index % 2 == 0) {
                                    color_idx = (state->grid.grid_color[byte_idx] >> 4) & 0x0F; // High nibble
                                } else {
                                    color_idx = state->grid.grid_color[byte_idx] & 0x0F;        // Low nibble
                                }

                                // 4. DRAW THE PLACED BLOCK
                                DrawRectangleRec(cell, state->utility.palette[color_idx]);
                                DrawRectangleLinesEx(cell, 1.0f, ColorAlpha(BLACK, 0.2f));
                            } 
                            else if (CheckCollisionPointRec(virtualMouse, cell)) {
                                // Only show the hover highlight if the cell is EMPTY
                                DrawRectangleRec(cell, ColorAlpha(WHITE, 0.1f));
                            }
                        }
                    }

                    RenderCenteredText(TextFormat("Score: %i", state->session.current_score), 400, 25, RAYWHITE, virtual_width);


                    for (u8 i = 0; i < 3; i++) {
                        if (!state->session.is_active[i]) {
                            u8 composite_byte = state->session.deck_shape_color_bits[i];
                            u8 shape_idx = GET_SHAPE(composite_byte);
                            u8 color_idx = GET_COLOR(composite_byte);

                            // Get the 64-bit mask from your library
                            u64 shape_mask = SHAPE_LIBRARY[shape_idx];

                            Vector2 draw_pos = (state->session.is_dragging && state->session.dragging_slot_index == i) 
                                                ? state->session.drag_pos 
                                                : DECK_SLOTS[i];

                            Color c = state->utility.palette[color_idx];

                            // We check a 4x4 area (most shapes fit here)
                            for (int y = 0; y < 4; y++) {
                                for (int x = 0; x < 4; x++) {
                                    // Calculate which bit to check
                                    // Bit 63 is (0,0), Bit 62 is (1,0)... Bit 56 is (7,0)
                                    int bit_index = 63 - (y * 8 + x);
                                    
                                    if ((shape_mask >> bit_index) & 1) {
                                        // Draw the block part
                                        float block_x = draw_pos.x + (x * cellSize) - (cellSize / 2.0f);
                                        float block_y = draw_pos.y + (y * cellSize) - (cellSize / 2.0f);

                                        DrawRectangle(block_x, block_y, cellSize, cellSize, c);
                                        DrawRectangleLinesEx((Rectangle){block_x, block_y, (float)cellSize, (float)cellSize}, 2.0f, ColorAlpha(BLACK, 0.3f));
                                    }
                                }
                            }
                        }
                    }
                   


                    break;

                case 2: 
                // render game over screen


                    break;
                
                case 3:
                // render settings
                    break;


                
                default:
                    // broken case
                    ClearBackground(RED);
                    DrawText("CRITICAL ERROR state->utility.current_screen is corrupted", 20, 20, 20, RAYWHITE);
            
                    //if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state->utility.current_screen = 0;
                    break;
            
            }

        EndTextureMode();


        BeginDrawing();
        ClearBackground(BLACK); // This fills the "Letterbox" area if the screen is wide

        // DrawTexturePro handles the "Casting" and Scaling
        DrawTexturePro(target.texture, 
            (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, // The Source (Canvas)
            (Rectangle){ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() },        // The Dest (Monitor)
            (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();

    }

    CloseWindow();
    return 0;


}