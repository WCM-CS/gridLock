#include <stdio.h>
#include "bg64_state.h"


int main(void) 
{
    printf("Initializing game state!\n");

    // load in game sate from files, user high sore and entire game state
    // load new seed each session for block gen uniqueness
    Arena game_arena = GameArena_Initialization(ARENA_SIZE);
    GameState *state = GameState_Initialization(&game_arena);


    fill_queue(state);

    u8 buff[256] = {0};

    u8 data1 = ring_buffer_consume_batch(&state->buffer, buff, 3);

    fill_queue(state);

    u8 data2 = ring_buffer_consume_batch(&state->buffer, buff, 255);


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
            DrawText(TextFormat("Score: %llu", state->user.current_score), 10, 40, 20, GREEN);
        EndDrawing();

        
    }

    CloseWindow();
    return 0;

}