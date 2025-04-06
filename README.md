
# ⌨️ CHIP-8 Emulator
A modern emulator for the classic **CHIP-8** virtual machine, built from scratch in **C++** with **SDL2**.  
Faithfully executes CHIP-8 programs, including classic games and official test ROMs.  
Supports display, input, timers, and full opcode coverage.

🧪 Uses:
- ROMs from [`Timendus/chip8-test-suite`](https://github.com/Timendus/chip8-test-suite)
- Documentation from [`trapexit/chip-8_documentation`](https://github.com/trapexit/chip-8_documentation)

🎮 Test example: `roms/3-corax+.ch8`

---

Want to play, test, or extend a tiny virtual machine from the 1970s? You’re in the right place.

---

## 📦 Test ROMs & Documentation

This emulator uses test ROMs from the excellent [Timendus CHIP-8 Test Suite](https://github.com/Timendus/chip8-test-suite)  
Documentation and opcode references from: [trapexit/chip-8_documentation](https://github.com/trapexit/chip-8_documentation)

---

### 🎮 ROM: `roms/3-corax+.ch8`

The opcodes rendered on screen during this test represent the following instructions:

```
3xnn    2nnn    8xy4    Fx55  
4xnn    00EE    8xy5    Fx33  
5xy0    8xy0    8xy7    Fx1E  
7xnn    8xy1    8xy6    Registers  
9xy0    8xy2    8xyE  
1nnn    8xy3    Fx65
```


## 🛠 How to Build and Run

Make sure you have SDL2 and CMake installed. Then run:

```bash
cmake .
make
./sdl2_project