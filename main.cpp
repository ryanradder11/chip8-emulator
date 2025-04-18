#include <iostream>
#include <fstream>
#include <vector>
#include <SDL2/SDL.h>

bool debug = false;

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

void printGFX(const Chip8& chip) {
    std::cout << "\n===== DISPLAY BUFFER =====\n";
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            std::cout << (chip.gfx[y * 64 + x] ? "█" : " ");
        }
        std::cout << "\n";
    }
    std::cout << "==========================\n";
}

void printRom(const Chip8& chip) {
    for (int i = 0; i < 1000; ++i) {
        uint16_t addr = 0x200 + (i * 2);
        uint16_t op = (chip.memory[addr] << 8) | chip.memory[addr + 1];
        std::cout << "0x" << std::hex << addr << ": " << std::hex << op << std::endl;
    }
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

    if (debug) {

    // Print the opcode in hexadecimal format
    std::cout << "Opcode (hex): " << std::hex << opcode << std::endl;

    // Print the opcode in binary format
    std::cout << "Opcode (binary): ";
    for (int i = 15; i >= 0; --i) {
        std::cout << ((opcode >> i) & 1);
    }
    std::cout << std::endl;
    }

    // 2. Decode and execute
    switch (opcode & 0xF000) {
        case 0x0000: {
            switch (opcode & 0x00FF) {
                case 0xE0: {
                    // 00E0 - Clear screen
                    memset(chip.gfx, 0, sizeof(chip.gfx));
                    chip.drawFlag = true;
                    chip.pc += 2;
                    if (debug) std::cout << "Clear screen\n";
                    break;
                }
                case 0xEE: {
                    // 00EE - Return from subroutine
                    chip.sp--;
                    chip.pc = chip.stack[chip.sp];
                    chip.pc += 2;
                    if (debug) std::cout << "Return from subroutine to 0x" << std::hex << chip.pc << "\n";
                    break;
                }
                default: {
                    std::cout << "Unknown 0x0000 opcode: " << std::hex << opcode << "\n";
                    break;
                }
            }
            break;
        }
        break;
        case 0x1000:
            chip.pc = opcode & 0x0FFF; // Jump to address NNN
            if (debug) std::cout << "Jump to address: " << std::hex << chip.pc << std::endl;
        break;
        case 0x2000:
            // 2NNN: Call subroutine at NNN
        {
            uint16_t address = opcode & 0x0FFF;
            chip.stack[chip.sp] = chip.pc; // Store current PC in the stack
            chip.sp++; // Increment stack pointer
            chip.pc = address; // Set PC to the address
            if (debug) std::cout << "Call subroutine at address: " << std::hex << address << std::endl;
        }
        break;
        case 0x3000:
            // 3XNN: Skip next instruction if Vx == NN
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (debug) std::cout << "3XNN: Checking if V[" << (int)x << "] == " << (int)nn << " (V[" << (int)x << "] = " << (int)chip.V[x] << ")\n";
            if (chip.V[x] == nn) {
                if (debug) std::cout << "Condition true: skipping next instruction.\n";
                chip.pc += 4;
            } else {
                if (debug) std::cout << "Condition false: proceeding to next instruction.\n";
                chip.pc += 2;
            }
        }
        break;
        case 0x4000:
            // 4XNN: Skip next instruction if Vx != NN
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (debug) std::cout << "4XNN: Checking if V[" << (int)x << "] != " << (int)nn << " (V[" << (int)x << "] = " << (int)chip.V[x] << ")\n";
            if (chip.V[x] != nn) {
                if (debug) std::cout << "Condition true: skipping next instruction.\n";
                chip.pc += 4;
            } else {
                if (debug) std::cout << "Condition false: proceeding to next instruction.\n";
                chip.pc += 2;
            }
        }
        break;
        case 0x5000:
            // 5XY0: Skip next instruction if VX == VY
                if ((opcode & 0x000F) == 0x0000) {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    if (debug) std::cout << "5XY0: Checking if V[" << (int)x << "] == V[" << (int)y << "] ("
                              << (int)chip.V[x] << " == " << (int)chip.V[y] << ")" << std::endl;

                    if (chip.V[x] == chip.V[y]) {
                        chip.pc += 4;
                    } else {
                        chip.pc += 2;
                    }
                } else {
                    std::cerr << "Unknown 0x5000 opcode variant: " << std::hex << opcode << std::endl;
                    chip.pc += 2;
                }
        break;
        case 0x6000:
            // 6XNN: Set Vx = NN
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            chip.V[x] = nn;
            chip.pc += 2;
        }
        if (debug) std::cout << "Set V" << (int)((opcode & 0x0F00) >> 8) << " = " << (int)(opcode & 0x00FF) << std::endl;
        break;
        case 0x7000:
            // 7XNN: Add NN to Vx
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            chip.V[x] += nn;
            chip.pc += 2;
        }
        if (debug) std::cout << "Add " << (int)(opcode & 0x00FF) << " to V" << (int)((opcode & 0x0F00) >> 8) << std::endl;
        break;
        case 0x8000: {
            //all 0x8000 opcodes share the same X,Y
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            switch (opcode & 0x000F) {
                case 0x0: {
                    //8XY0	LD VX, VY Copy the value in register VY into VX
                    chip.V[x] = chip.V[y];
                    chip.pc += 2;
                    if (debug) std::cout << "Set V[" << (int)x << "] = V[" << (int)y << "] (" << (int)chip.V[y] << ")\n";
                    break;
                }
                // 8XY1 - Sets VX to (VX OR VY).
                case 0x1: {
                    chip.V[x] |= chip.V[y];
                    chip.pc += 2;
                    if (debug) std::cout << "Set V[" << (int)x << "] (OR)|= V[" << (int)y << "] (" << (int)chip.V[y] << ")\n";
                }
                break;
                // Set VX equal to the bitwise and of the values in VX and VY.
                case 0x2: {
                    chip.V[x] = chip.V[x] & chip.V[y];
                    chip.pc += 2;
                    if (debug) std::cout <<"Set V[" << (int)x << "] &= V[" << (int)y << "] (" << (int)chip.V[y] << ")\n";
                }
                break;
                // Set VX equal to the bitwise xor of the values in VX and VY
                case 0x3: {
                    chip.V[x] = chip.V[x] ^ chip.V[y];
                    chip.pc += 2;
                    if (debug) std::cout <<"Set V[" << (int)x << "] (XOR)^= V[" << (int)y << "] (" << (int)chip.V[y] << ")\n";
                }
                // Set VX equal to VX plus VY. In the case of an overflow(carry) VF is set to 1. Otherwise 0.
                case 0x4: {
                    chip.V[x] = (chip.V[x] + chip.V[y]) &0xff;
                    if (chip.V[y] > chip.V[x]) {
                        chip.V[0xf] = 1;
                    }else {
                        chip.V[0xf] = 0;
                    }
                    chip.pc += 2;
                }
                    if (debug) std::cout << "Set V[" << (int) x << "] += V[" << (int) y << "] (" << (int) chip.V[y] <<
                            "), with carry\n";
                break;
                // Set VX equal to VX minus VY. In the case of an underflow VF is set 0. Otherwise 1. (VF = VX > VY)
                case 0x5: {
                    chip.V[0xF] = (chip.V[x] >= chip.V[y]) ? 1 : 0;
                    chip.V[x] = (chip.V[x] - chip.V[y]) & 0xFF;
                    chip.pc += 2;
                    if (debug) std::cout << "Set V[" << (int)x << "] -= V[" << (int)y << "] (" << (int)chip.V[y] << "), with borrow flag\n";
                }
                //Set VX equal to VX bitshifted right 1. VF is set to the least significant bit of VX prior to the shift.
                case 0x6: {
                    chip.V[0xF] = chip.V[x] & 0x1;
                    chip.V[x] = chip.V[x] >> 1;
                    chip.pc += 2;
                    if (debug) std::cout << "Shift V[" << (int)x << "] right by 1. VF = " << (int)chip.V[0xF] << "\n";
                }
                break;
                //Set VX equal to VY minus VX. VF is set to 1 if VY > VX. Otherwise 0.
                case 0x7: {
                    chip.V[0xF] = chip.V[y] > chip.V[x] ? 1 : 0;
                    chip.V[x] = chip.V[y] - chip.V[x] & 0xFF;
                    chip.pc += 2;
                    if (debug) std::cout << "Set V[" << (int)x << "] = V[" << (int)y << "] - V[" << (int)x << "] ("
                            << (int) chip.V[y] << " - " << (int) chip.V[x] << "), VF = "
                            << (int) chip.V[0xF] << "\n";
                }
                break;
                // Set VX equal to VX bitshifted left 1. VF is set to the most significant bit of VX prior to the shift
                case 0xe: {
                    uint8_t mostSignificantBit = (chip.V[x] & 0x80) ? 1 : 0;
                    chip.V[0xF] = mostSignificantBit;
                    chip.V[x] = (chip.V[x] << 1) & 0xFF;
                    chip.pc += 2;
                    if (debug) std::cout << "Shift V[" << (int)x << "] left by 1. VF = " << (int)chip.V[0xF] << "\n";
                }
                break;
                default:
                    std::cerr << "Unknown 8XY opcode: " << std::hex << opcode << std::endl;
                    chip.pc += 2;
                break;
            }
        }
        break;
        case 0x9000:
            // 9XY0: Skip next instruction if Vx != Vy
            if ((opcode & 0x000F) == 0x0000) {
                uint8_t x = (opcode & 0x0F00) >> 8;
                uint8_t y = (opcode & 0x00F0) >> 4;
                if (debug) std::cout << "9XY0: Checking if V[" << (int) x << "] != V[" << (int) y << "] ("
                        << (int) chip.V[x] << " != " << (int) chip.V[y] << ")" << std::endl;
                if (chip.V[x] != chip.V[y]) {
                    chip.pc += 4;
                } else {
                    chip.pc += 2;
                }
            } else {
                std::cerr << "Unknown 0x9000 opcode: " << std::hex << opcode << std::endl;
                chip.pc += 2;
            }
            break;
        // Set I equal to NNN.
        case 0xA000:
            chip.I = opcode & 0x0FFF;
            chip.pc += 2;
            if (debug) std::cout << "Set I = " << std::hex << chip.I << std::endl;
            break;
        // Set the PC to NNN plus the value in V0.
        case 0xB000:
            chip.pc = opcode & 0x0FFF + chip.V[0];
            if (debug) std::cout << "Set PC= " << std::hex << chip.pc << std::endl;
            break;
        // Set VX equal to a random number ranging from 0 to 255 which is logically anded with NN.
        case 0xC000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            uint8_t rnd = rand() % 256;
            chip.V[x] = rnd & nn;
            chip.pc += 2;
            if (debug) std::cout << "Set V[" << (int)x << "] = rand() & 0x"
                      << std::hex << (int)nn << " => " << std::dec
                      << (int)chip.V[x] << "\n";
        }
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
        if (debug) std::cout << "Draw sprite at (" << (int)chip.V[(opcode & 0x0F00) >> 8] << ", " << (int)chip.V[(opcode & 0x00F0) >> 4] << ")" << std::endl;
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
        if (debug) std::cout << "Skip next instruction if key with value of V" << (int)((opcode & 0x0F00) >> 8) << " is pressed" << std::endl;
        if (debug) std::cout << "VX value: " << (int)chip.V[(opcode & 0x0F00) >> 8] << std::endl;
        break;
        case 0xF000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            switch (opcode & 0x00FF) {
                // Set VX equal to the delay timer.
                case 0x07: {
                    chip.V[x] = chip.delay_timer;
                    chip.pc += 2;
                    if (debug) std::cout << "Set V[" << (int)x << "] = delay_timer (" << (int)chip.delay_timer << ")\n";
                }
                break;
                // FX18: Set sound timer = Vx
                case 0x0018:
                {
                    chip.sound_timer = chip.V[x];
                    chip.pc += 2;
                    if (debug) std::cout << "Set sound timer = V" << (int)x << std::endl;
                }
                break;
                // FX15 set delay timer = Vx
                case 0x0015: {
                    chip.delay_timer = chip.V[x];
                    chip.pc += 2;
                    if (debug) std::cout << "Set delay timer = V" << (int)x << std::endl;
                }
                break;
                // Set I to the address of the CHIP-8 8x5 font sprite representing the value in VX.
                case 0x0029: { // Fx29
                    chip.I = chip.V[x] * 5; // assuming font sprites start at address 0
                    chip.pc += 2;
                    if (debug) std::cout << "Set I to sprite address for character in V[" << std::dec << (int)x << "] = "
                              << std::hex << (int)chip.V[x] << ", I = " << chip.I << "\n";
                }
                break;
                // Convert that word to BCD and store the 3 digits at memory location I through I+2. I does not change.
                case 0x33: {
                    uint8_t value = chip.V[x];
                    chip.memory[chip.I]     = value / 100;
                    chip.memory[chip.I + 1] = (value / 10) % 10;
                    chip.memory[chip.I + 2] = value % 10;
                    chip.pc += 2;

                    if (debug) std::cout << "Stored BCD of V[" << (int)x << "] (" << (int)value
                              << ") into memory at I, I+1, and I+2\n";
                }
                break;
                // Store registers V0 through Vx in memory starting at address I.
                case 0x0055: {
                    for (int i = 0; i <= x; ++i) {
                        chip.memory[chip.I + i] = chip.V[i];
                    }
                    chip.pc += 2;

                    if (debug) std::cout << "Stored V[0] to V[" << (int)x << "] into memory starting at I (0x"
                              << std::hex << chip.I << ")\n";
                }
                break;
                // Copy values from memory location I through I + X into registers V0 through VX. I does not change.
                case 0x0065: {
                    for (int i = 0; i <= x; i++) {
                        chip.V[i] = chip.memory[chip.I + i];
                    }
                    chip.pc += 2;
                    if (debug) std::cout << "Read V[0] to V[" << (int) x << "] from memory starting at I (0x"
                            << std::hex << chip.I << ")\n";
                }
                break;
                // Add VX to I. VF is set to 1 if I > 0x0FFF. Otherwise set to 0.
                case 0x1E: {
                    uint16_t sum = chip.I + chip.V[x];
                    chip.V[0xF] = (sum > 0x0FFF) ? 1 : 0;
                    chip.I = sum & 0x0FFF;
                    chip.pc += 2;

                    if (debug) std::cout << "Add V[" << (int)x << "] (" << (int)chip.V[x] << ") to I. VF = "
                              << (int)chip.V[0xF] << ", New I = 0x" << std::hex << chip.I << "\n";
                }
                break;
                case 0x0080:
                    //TODO
                        // FF80: Custom opcode - treat as NOP or marker
                            std::cout << "Custom opcode FF80 encountered. (Possible sprite data marker?)" << std::endl;
                chip.pc += 2;
                break;
                // Add other 0xF000 opcodes here...
                default:
                    std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
                break;
            }
        }
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
    loadROM("roms/games/Figures.ch8", chip); // Replace with your ROM path
    bool quit = false;
    SDL_Event e;
    uint32_t lastTimerUpdate = SDL_GetTicks();

    //Dump ROM memory for debugging
    // printRom(chip);

    while (!quit) {
        // 1. Process input
        processInput(chip, quit);

        // 2. Emulate N cycles per frame
        for (int i = 0; i < 1; ++i) {
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

            // print GFX in terminal for debugging purpose
            // printGFX(chip);
        }

        SDL_Delay(10); // Fine-tune delay for control
        if (debug)std::cout << "======= end tick =======\n";
    }

    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

