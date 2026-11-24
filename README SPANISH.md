# PAC-MAN | C++ Arcade Edition

A faithful, optimized clone of the classic Pac-Man, written in modern C++17 and compiled natively for Windows. Features differentiated AI for all four ghosts, retro synthesized audio, optional WAV file support, and flicker-free console rendering.

## Key Features

- **Authentic arcade gameplay**: Classic 29×21 maze with pellets, power pellets, tunnels, and ghost house
- **Four ghosts with unique AI**:
  - **Blinky (Red)**: Directly chases Pac-Man
  - **Pinky (Pink)**: Ambushes by targeting 4 steps ahead of Pac-Man
  - **Inky (Cyan)**: Seeks alternate routes using reflective geometry
  - **Clyde (Orange)**: Alternates between chase and scatter based on distance
- **Dual audio system**: Mathematically synthesized effects (zero dependencies) + optional WAV file loading (`sonidos/sirena.wav`, `sonidos/comer.wav`)
- **Optimized rendering**: Double-buffered `WriteConsoleOutput` for stable 60 FPS without flicker
- **Efficient BFS pathfinding**: Computes real corridor distances every tick
- **Responsive controls**: Direction buffering (allows turning before reaching corners)
- **Scoring & combos**: 10 pts/pellet, 50 pts/power pellet, 200→400→800→1600 pts for chained ghosts in power mode
- **Three lives**, pause (P), restart (R), main menu, mute sound (M)

## Game Mechanics

| Element | Description |
|---------|-------------|
| Pellets (`.`) | 10 points each, 240 total |
| Power Pellets (`o`) | 50 points, activate **power mode** (7.8 s) |
| Power mode | Ghosts turn blue/vulnerable, points ×2ⁿ per chain |
| Eaten ghost | Returns to house, 18-tick respawn delay |
| Side tunnels | Wrap-around between x=0 and x=28 |
| Victory | Collect all pellets |
| Defeat | Lose all 3 lives |

## Controls

| Key | Action |
|-----|--------|
| `W` / `↑` | Up |
| `S` / `↓` | Down |
| `A` / `←` | Left |
| `D` / `→` | Right |
| `P` | Pause / Resume |
| `R` | Restart game |
| `M` | Toggle sound on/off |
| `ESC` | Return to menu / Exit |
| `1` / `2` | Direct menu selection |

## System Requirements

- **Windows 10** (1903+) or **Windows 11**
- **x64 architecture**
- Console with ANSI color support (Windows Terminal, modern CMD, PowerShell 7+)
- **Zero external dependencies** (only system `winmm.dll`)

## Download & Run

1. Download `Pacman.exe` from the [`dist/`](dist/) folder
2. (Optional) Place `sirena.wav` and `comer.wav` in a `sonidos/` subfolder next to the executable
3. Double-click `Pacman.exe` or run from terminal

## Build from Source

### Build Requirements
- **Visual Studio 2022** (Community, Professional, or Enterprise) with **"Desktop development with C++"** workload
- Or **Visual Studio Build Tools 2022** (lightweight install)

### Steps

```cmd
git clone https://github.com/your-user/pacman-cpp.git
cd pacman-cpp
build.bat
```

The optimized executable (`Pacman.exe`, ~270 KB) will be generated in `dist/`.

### Release Build Flags
- `/O2` — Maximum speed optimization
- `/GL` — Link-time code generation (LTCG)
- `/OPT:REF /OPT:ICF` — Dead code elimination and identical COMDAT folding
- `/std:c++17` — C++17 standard
- `/EHsc` — Synchronous exception handling
- `/W4` — High warning level (with minor suppressions)

## Project Structure

```
PACMAN/
├── src/
│   └── pacman.cpp      # Complete source code (single-file)
├── sonidos/            # Optional: sirena.wav, comer.wav
├── dist/
│   └── Pacman.exe      # Compiled executable (Release)
├── build.bat           # Automated build script
├── README.md           # Spanish documentation
├── README_EN.md        # This file (English)
└── .gitignore          # Git ignore rules
```

## Technologies Used

- **C++17** — Modern standard, zero external library dependencies
- **Windows API** — Console (`WriteConsoleOutput`), audio (`WinMM`), threading (`<thread>`)
- **STL** — `array`, `vector`, `queue`, `random`, `chrono`, `algorithm`
- **MSVC** — Microsoft's optimizing compiler (cl.exe + link.exe)

## Credits

- **Original game**: *Pac-Man* (Namco, 1980) — Tōru Iwatani, designer
- **C++ implementation**: Modern software engineering applied to classic arcade
- **Audio**: Mathematical FM synthesis (no samples) + optional WAV support
- **Algorithms**: BFS for grid-graph distances, AI based on *Pac-Man Dossier* (Jamey Pittman)

---

*Educational / portfolio project. Open source under MIT license.*