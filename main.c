#include <stdio.h>
#include <math.h>
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

    ring_buffer_consume_batch(state, state->session.deck_shape_color_bits, 3);
    while(!WindowShouldClose()) {

        // 1: GETTING USER IO; Input + Coordinates
        Vector2 mouse = GetMousePosition();
        Vector2 virtualMouse = {
            mouse.x * (f32)virtual_width / GetScreenWidth(),
            mouse.y * (f32)virtual_height / GetScreenHeight()
        };



        // Logic loop
        // in game screen check if button is down


        // pick up block

        switch (state->utility.current_screen) {
            case 0: // Main screen
                UpdateMenus(state, virtualMouse);
                break;
            case 1: // Game screen
                UpdateGameLogic(state, virtualMouse, offsetX, offsetY, cellSize);
                break;
            case 2: // Game lost
            
                break;
            case 3: // Settings
            
                break;
            
            default:
                break;
        }


        // Render to 
        BeginTextureMode(target);
            ClearBackground(DARKGRAY);

            // Collect game logic
            switch (state->utility.current_screen) {
                case 0:
                    RenderMainScreen(state, virtual_width, virtualMouse);
                    break;

                case 1:
                    RenderGameScreen(state, offsetX, offsetY, cellSize, virtual_width);
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
