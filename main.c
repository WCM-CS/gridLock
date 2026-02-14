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







    //fill_queue(state);

    u8 buff[256] = {0};

    u8 data1 = ring_buffer_consume_batch(state, buff, 24);

    fill_queue(state);

    u8 idx = 0;
    loop {
        if (idx >= data1) break;
        u8 comp_byte = buff[idx];

        u8 shape_id = GET_SHAPE(comp_byte);
        u8 color_id = GET_COLOR(comp_byte);

        printf("Slot %d -> Shape ID: %d, Color ID: %d\n", idx, shape_id, color_id);
        
        idx++;
    }

    // Window
    InitWindow(360, 780, "Blocks");
    SetTargetFPS(60);


    // Game loop
    loop {
        if (WindowShouldClose()) break;
        

        if(IsKeyPressed(KEY_S)) {
            printf("Saving state...\n"); // Add a print so you can see it work!
            save_state("save.bin", state);
        }
        
        if (IsKeyPressed(KEY_L)) {
            printf("Loading state...\n");
            usize bytes = load_state("save.bin", state);
        }

        

        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Press S to save, L to load", 10, 10, 20, RAYWHITE);
            DrawText(TextFormat("Score: %llu", state->session.current_score), 10, 40, 20, GREEN);
        EndDrawing();

        
    }

    CloseWindow();
    return 0;

}