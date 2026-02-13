#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bg64_state.h"


int main(void) 
{
    printf("Initializing game state!\n");

    // load in game sate from files, user high sore and entire game state

    Arena game_arena = GameArena_Initialization(ARENA_SIZE);
    GameState *state = GameState_Initialization(&game_arena);



    // Window
    InitWindow(360, 780, "Blocks");
    SetTargetFPS(60);


    // Game loop
    loop
    {
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