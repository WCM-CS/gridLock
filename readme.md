Game Name: gridLock
- 8x8 Grid tetris style block game.

Engine Name: BG64 (64 Bit Grid)
- Game Engine includes high performance bitboarding, ring buffer queues, and u8 bit packing techniques; fitting the entire game state into 4 cache lines. We use raylib graphics library for rendering.

To set up the project locally you need to follow these steps.

// Configuration Note
This project directory includes a nix shell defining all the libraries and compilation tools we need for this project to run on Linux x86 machines. You either need to be on NixOs or have Nix package manager installed locally. Once installed you simply run "nix-shell" command to enter the shell, this may take a minute when you first fetch the world but it will save state to reuse packages next time. After the shell launches run "code ." to open vscode from the shell. F5 runs the code in the debugger, ctrl + F5 runs the code without debugging.

// Configuration Steps
1. Clone the repo
2. Build nix shell via "nix-shell" command
3. Launch vscode with "code ." command
4. Run & Debug code: F5 to start debugger, ctrl + F5 run with no debugger

