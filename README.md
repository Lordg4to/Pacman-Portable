# PAC-MAN | C++ Arcade Edition

Un clon fiel y optimizado del clásico Pac-Man, escrito en C++17 moderno y compilado nativamente para Windows. Incluye inteligencia artificial diferenciada para los cuatro fantasmas, sonidos sintetizados retro, compatibilidad con archivos WAV externos y renderizado de consola sin parpadeo.

## Características principales

- **Jugabilidad arcade auténtica**: Laberinto clásico 29×21 con puntos, superpuntos, túneles y casa de fantasmas
- **Cuatro fantasmas con IA única**:
  - **Blinky (Rojo)**: Persigue directamente a Pac-Man
  - **Pinky (Rosa)**: Intenta anticiparse moviéndose 4 pasos por delante
  - **Inky (Cian)**: Busca rutas alternativas usando geometría reflectiva
  - **Clyde (Naranja)**: Alterna persecución y retirada según la distancia
- **Sistema de audio dual**: Efectos sintetizados matemáticamente (sin dependencias) + carga opcional de archivos WAV (`sonidos/sirena.wav`, `sonidos/comer.wav`)
- **Renderizado optimizado**: Double-buffering con `WriteConsoleOutput` para 60 FPS estables sin parpadeo
- **Búsqueda de caminos BFS eficiente**: Calcula distancias reales por pasillos en cada tick
- **Controles responsivos**: Buffer de dirección anticipada (permite girar antes de llegar a la esquina)
- **Puntuación y combos**: 10 pts/punto, 50 pts/superpunto, 200→400→800→1600 pts por fantasmas encadenados en modo poder
- **Tres vidas**, pausa (P), reinicio (R), menú principal, silenciar sonido (M)

## Mecánicas del juego

| Elemento | Descripción |
|----------|-------------|
| Puntos (`.`) | 10 puntos cada uno, 240 en total |
| Superpuntos (`o`) | 50 puntos, activan **modo poder** (7.8 s) |
| Modo poder | Fantasmas azules/vulnerables, puntos ×2ⁿ por cadena |
| Fantasma comido | Regresa a casa, 18 ticks de espera |
| Túneles laterales | Teletransporte entre x=0 y x=28 |
| Victoria | Recoger todos los puntos |
| Derrota | Perder las 3 vidas |

## Controles

| Tecla | Acción |
|-------|--------|
| `W` / `↑` | Arriba |
| `S` / `↓` | Abajo |
| `A` / `←` | Izquierda |
| `D` / `→` | Derecha |
| `P` | Pausa / Continuar |
| `R` | Reiniciar partida |
| `M` | Activar / Desactivar sonido |
| `ESC` | Volver al menú / Salir |
| `1` / `2` | Selección directa en menú |

## Requisitos del sistema

- **Windows 10** (1903+) o **Windows 11**
- **Arquitectura x64**
- Consola con soporte de colores ANSI (Windows Terminal, CMD moderno, PowerShell 7+)
- **Sin dependencias externas** (solo `winmm.dll` del sistema)

## Descargar y ejecutar

1. Descarga `Pacman.exe` de la carpeta [`dist/`](dist/)
2. (Opcional) Coloca `sirena.wav` y `comer.wav` en una subcarpeta `sonidos/` junto al ejecutable
3. Haz doble clic en `Pacman.exe` o ejecútalo desde terminal

## Compilar desde el código fuente

### Requisitos de compilación
- **Visual Studio 2022** (Community, Professional o Enterprise) con la carga de trabajo **"Desarrollo para el escritorio con C++"**
- O **Visual Studio Build Tools 2022** (instalación ligera)

### Pasos

```cmd
git clone https://github.com/tu-usuario/pacman-cpp.git
cd pacman-cpp
build.bat
```

El ejecutable optimizado (`Pacman.exe`, ~270 KB) se generará en `dist/`.

### Opciones de compilación (Release)
- `/O2` — Optimización máxima de velocidad
- `/GL` — Link-time code generation (LTCG)
- `/OPT:REF /OPT:ICF` — Eliminación de código muerto y plegado de funciones idénticas
- `/std:c++17` — Estándar C++17
- `/EHsc` — Manejo de excepciones sincrónico
- `/W4` — Nivel de advertencias alto (con supresiones menores)

## Estructura de carpetas

```
PACMAN/
├── src/
│   └── pacman.cpp      # Código fuente completo (single-file)
├── sonidos/            # Opcional: sirena.wav, comer.wav
├── dist/
│   └── Pacman.exe      # Ejecutable compilado (Release)
├── build.bat           # Script de compilación automatizado
├── README.md           # Este archivo (español)
├── README_EN.md        # Versión en inglés
└── .gitignore          # Archivos ignorados por Git
```

## Tecnologías utilizadas

- **C++17** — Estándar moderno, sin dependencias de bibliotecas externas
- **Windows API** — Consola (`WriteConsoleOutput`), audio (`WinMM`), hilos (`<thread>`)
- **STL** — `array`, `vector`, `queue`, `random`, `chrono`, `algorithm`
- **MSVC** — Compilador optimizador de Microsoft (cl.exe + link.exe)

## Créditos

- **Juego original**: *Pac-Man* (Namco, 1980) — Tōru Iwatani, diseñador
- **Implementación C++**: Ingeniería de software moderna aplicada a arcade clásico
- **Sonidos**: Síntesis FM matemática (sin samples) + soporte WAV opcional
- **Algoritmos**: BFS para distancias en grafo de rejilla, IA basada en *Pac-Man Dossier* (Jamey Pittman)

---

*Proyecto educativo / portfolio. Código abierto bajo licencia MIT.*