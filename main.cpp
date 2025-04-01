#include <iostream>
#include <fstream>
#include <vector>
#include <SDL2/SDL.h>

struct Chip8 {
    uint8_t memory[4096];
    uint8_t V[16];           // Registers V0 to VF
    uint16_t I;              // Index register
    uint16_t pc;             // Program counter
    uint8_t gfx[64 * 32];    // Display (monochrome 64x32)
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t stack[16];
    uint16_t sp;             // Stack pointer
    uint8_t keypad[16];      // Hex-based keypad (0x0-0xF)

    // Add init/load/execute functions later
};

bool loadROM(const char* filename, Chip8& chip) {
    // Open the file in binary mode and move the file pointer to the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        // If the file couldn't be opened, return false
        return false;
    }

    // Get the size of the file
    std::streamsize size = file.tellg();
    // Move the file pointer back to the beginning
    file.seekg(0, std::ios::beg);

    // Create a buffer to hold the file contents
    std::vector<char> buffer(size);
    // Read the file into the buffer
    if (file.read(buffer.data(), size)) {
        // Copy the buffer into the Chip8 memory starting at address 0x200
        for (size_t i = 0; i < size; ++i) {
            chip.memory[0x200 + i] = buffer[i];
        }
        // Return true if the file was successfully read
        return true;
    }

    // Return false if there was an error reading the file
    return false;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hello SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    std::cout << "Window created successfully!" << std::endl;

    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_Delay(16); // Delay to limit CPU usage
    }

    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

