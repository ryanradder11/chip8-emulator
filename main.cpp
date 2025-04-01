#include <iostream>
#include <fstream>
#include <vector>
#include <SDL2/SDL.h>

struct Chip8 {
    bool drawFlag; // Set to true if the screen needs to be redrawn
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
        std::cout << "Failed to open ROM: " << filename << std::endl;
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
        std::cout << "Successfully loaded ROM: " << filename << std::endl;
        return true;
    }

    // Return false if there was an error reading the file
    return false;
}

// Function to process input events
void processInput(Chip8& chip, bool& quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit = true;
        }

        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            bool isPressed = (event.type == SDL_KEYDOWN);

            std::cout << "Key event: " << event.key.keysym.sym << std::endl;
            switch (event.key.keysym.sym) {
                case SDLK_0: chip.keypad[0x0] = isPressed; break;
                case SDLK_1: chip.keypad[0x1] = isPressed; break;
                case SDLK_2: chip.keypad[0x2] = isPressed; break;
                case SDLK_3: chip.keypad[0x3] = isPressed; break;
                case SDLK_4: chip.keypad[0xC] = isPressed; break;

                case SDLK_q: chip.keypad[0x4] = isPressed; break;
                case SDLK_w: chip.keypad[0x5] = isPressed; break;
                case SDLK_e: chip.keypad[0x6] = isPressed; break;
                case SDLK_r: chip.keypad[0xD] = isPressed; break;

                case SDLK_a: chip.keypad[0x7] = isPressed; break;
                case SDLK_s: chip.keypad[0x8] = isPressed; break;
                case SDLK_d: chip.keypad[0x9] = isPressed; break;
                case SDLK_f: chip.keypad[0xE] = isPressed; break;

                case SDLK_z: chip.keypad[0xA] = isPressed; break;
                case SDLK_x: chip.keypad[0x0] = isPressed; break;
                case SDLK_c: chip.keypad[0xB] = isPressed; break;
                case SDLK_v: chip.keypad[0xF] = isPressed; break;
            }
        }
    }
}

void emulateCycle(Chip8 &chip) {
    // 1. Fetch opcode
    uint16_t opcode = chip.memory[chip.pc] << 8 | chip.memory[chip.pc + 1];
    // Print the opcode in hexadecimal format
    std::cout << "Opcode (hex): " << std::hex << opcode << std::endl;

    // Print the opcode in binary format
    std::cout << "Opcode (binary): ";
    for (int i = 15; i >= 0; --i) {
        std::cout << ((opcode >> i) & 1);
    }
    std::cout << std::endl;

    // 2. Decode and execute
    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                // Clear screen
                memset(chip.gfx, 0, sizeof(chip.gfx));
                chip.drawFlag = true;
                chip.pc += 2;
            }
        std::cout << "Clear screen" << std::endl;
        break;
        case 0x1000:
            chip.pc = opcode & 0x0FFF; // Jump to address NNN
            std::cout << "Jump to address: " << std::hex << chip.pc << std::endl;
        break;
        case 0x6000:
            // 6XNN: Set Vx = NN
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            chip.V[x] = nn;
            chip.pc += 2;
        }
        std::cout << "Set V" << (int)((opcode & 0x0F00) >> 8) << " = " << (int)(opcode & 0x00FF) << std::endl;
        break;
        case 0x7000:
            // 7XNN: Add NN to Vx
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            chip.V[x] += nn;
            chip.pc += 2;
        }
        std::cout << "Add " << (int)(opcode & 0x00FF) << " to V" << (int)((opcode & 0x0F00) >> 8) << std::endl;
        break;
        case 0xA000:
            // ANNN: Set I = NNN
                chip.I = opcode & 0x0FFF;
        chip.pc += 2;
        break;

        case 0xD000:
            // DXYN: Draw sprite at (Vx, Vy) with width 8 pixels and height N pixels
        {
            uint8_t x = chip.V[(opcode & 0x0F00) >> 8];
            uint8_t y = chip.V[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;

            // Reset the collision flag
            chip.V[0xF] = 0;

            // Iterate over each row of the sprite
            for (int yline = 0; yline < height; yline++) {
                // Get the pixel data for the current row
                uint8_t pixel = chip.memory[chip.I + yline];

                // Iterate over each bit in the row (8 bits per row)
                for (int xline = 0; xline < 8; xline++) {
                    // Check if the current bit is set (pixel is on)
                    if ((pixel & (0x80 >> xline)) != 0) {
                        // Calculate the index in the display buffer
                        int index = x + xline + ((y + yline) * 64);

                        // Check for collision (if the pixel is already on)
                        if (chip.gfx[index] == 1) {
                            chip.V[0xF] = 1; // Set collision flag
                        }

                        // XOR the pixel (toggle it)
                        chip.gfx[index] ^= 1;
                    }
                }
            }

            chip.drawFlag = true;
            chip.pc += 2;
        }
        std::cout << "Draw sprite at (" << (int)chip.V[(opcode & 0x0F00) >> 8] << ", " << (int)chip.V[(opcode & 0x00F0) >> 4] << ")" << std::endl;
        break;
        case 0xE000:
            // EX9E: Skip next instruction if key with value of Vx is pressed
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            if (chip.V[x] < 16 && chip.keypad[chip.V[x]] == 1) {
                std::cout << "Key pressed: " << (int)chip.V[x] << std::endl;
                chip.pc += 4;
            } else {
                chip.pc += 2;
            }
        }
        std::cout << "Skip next instruction if key with value of V" << (int)((opcode & 0x0F00) >> 8) << " is pressed" << std::endl;
        std::cout << "VX value: " << (int)chip.V[(opcode & 0x0F00) >> 8] << std::endl;
        break;
        // Add more ...
        default:
            std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
        break;
    }

    // 3. Update timers (usually done in main loop every 60Hz)
}

void drawDisplay(SDL_Renderer* renderer, const uint8_t* gfx, int scale = 10) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White pixels

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            if (gfx[y * 64 + x]) {
                SDL_Rect pixel = { x * scale, y * scale, scale, scale };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Chip8 emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    std::cout << "Window created successfully!" << std::endl;

    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;

    }
    Chip8 chip;
    chip.pc = 0x200; // Start of most CHIP-8 programs
    loadROM("roms/tron.ch8", chip); // Replace with your ROM path
    bool quit = false;
    SDL_Event e;
    uint32_t lastTimerUpdate = SDL_GetTicks();

    while (!quit) {
        // 1. Process input

            processInput(chip, quit);

        // 2. Emulate N cycles per frame
        for (int i = 0; i < 10; ++i) {
            emulateCycle(chip);
        }

        // 3. Update timers every 16ms (60Hz)
        if (SDL_GetTicks() - lastTimerUpdate >= 16) {
            if (chip.delay_timer > 0) chip.delay_timer--;
            if (chip.sound_timer > 0) {
                if (--chip.sound_timer == 0) {
                    // TODO: play sound
                }
            }
            lastTimerUpdate = SDL_GetTicks();
        }

        // 4. Draw if needed
        if (chip.drawFlag) {
            drawDisplay(renderer, chip.gfx);
            chip.drawFlag = false;
        }

        SDL_Delay(330); // Fine-tune delay for control
    }

    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

