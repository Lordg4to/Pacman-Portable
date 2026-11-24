#define NOMINMAX
#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <queue>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <fstream>
#include <iterator>
#include <cstring>
#include <windows.h>
#include <conio.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;

const int W = 29, H = 21;
const int POWER_TIME = 65;
const int TICK_MS = 120;

enum class Sfx { Select, Start, Pellet, PelletAlt, Power, Ghost, Death, Win, Lose, Count };

class Audio {
    struct Tone { double from, to; int ms; };
    static constexpr int Rate = 22050;
    using Bytes = vector<unsigned char>;
    array<Bytes, static_cast<size_t>(Sfx::Count)> clips;
    array<int, static_cast<size_t>(Sfx::Count)> duration{};
    chrono::steady_clock::time_point until{};
    int currentPriority = 0;
    bool enabled = true;
    bool alternatePellet = false;
    bool externalPellet = false;

    Bytes sirenPCM;
    WAVEFORMATEX sirenFormat{};
    HWAVEOUT sirenDevice = nullptr;
    WAVEHDR sirenHeader{};
    bool backgroundAttempted = false;

    static uint32_t readNumber(const Bytes& b, size_t offset, int count) {
        uint32_t v = 0;
        for (int i = 0; i < count; ++i)
            v |= static_cast<uint32_t>(b[offset + i]) << (i * 8);
        return v;
    }

    static bool loadWav(const char* path, Bytes& file, Bytes& pcm, WAVEFORMATEX& format) {
        ifstream in(path, ios::binary);
        if (!in) return false;
        Bytes b((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        if (b.size() < 44 || memcmp(b.data(), "RIFF", 4) || memcmp(b.data() + 8, "WAVE", 4)) return false;
        WAVEFORMATEX f{};
        size_t dataStart = 0, dataSize = 0;
        bool hasFormat = false;
        for (size_t pos = 12; pos + 8 <= b.size();) {
            size_t length = readNumber(b, pos + 4, 4);
            size_t start = pos + 8;
            if (length > b.size() - start) return false;
            if (!memcmp(b.data() + pos, "fmt ", 4) && length >= 16) {
                f.wFormatTag = static_cast<WORD>(readNumber(b, start, 2));
                f.nChannels = static_cast<WORD>(readNumber(b, start + 2, 2));
                f.nSamplesPerSec = readNumber(b, start + 4, 4);
                f.nAvgBytesPerSec = readNumber(b, start + 8, 4);
                f.nBlockAlign = static_cast<WORD>(readNumber(b, start + 12, 2));
                f.wBitsPerSample = static_cast<WORD>(readNumber(b, start + 14, 2));
                hasFormat = true;
            } else if (!memcmp(b.data() + pos, "data", 4) && !dataSize) {
                dataStart = start;
                dataSize = length;
            }
            pos = start + length;
            if ((length & 1) && pos < b.size()) ++pos;
        }
        if (!hasFormat || f.wFormatTag != WAVE_FORMAT_PCM ||
            (f.nChannels != 1 && f.nChannels != 2) ||
            (f.wBitsPerSample != 8 && f.wBitsPerSample != 16) ||
            f.nSamplesPerSec < 8000 || f.nSamplesPerSec > 96000 ||
            f.nBlockAlign != f.nChannels * f.wBitsPerSample / 8 ||
            f.nAvgBytesPerSec != f.nSamplesPerSec * f.nBlockAlign ||
            dataSize == 0 || dataSize % f.nBlockAlign != 0) return false;
        pcm.assign(b.begin() + dataStart, b.begin() + dataStart + dataSize);
        file.swap(b);
        format = f;
        return true;
    }

    void makeSiren() {
        sirenFormat.wFormatTag = WAVE_FORMAT_PCM;
        sirenFormat.nChannels = 1;
        sirenFormat.nSamplesPerSec = Rate;
        sirenFormat.wBitsPerSample = 16;
        sirenFormat.nBlockAlign = 2;
        sirenFormat.nAvgBytesPerSec = Rate * 2;
        const double pi = 3.141592653589793;
        for (int i = 0; i < Rate; ++i) {
            double t = static_cast<double>(i) / Rate;
            double phase = 2 * pi * 390 * t + 16 * sin(2 * pi * 3 * t);
            double wave = .78 * sin(phase) + .22 * sin(3 * phase);
            int16_t sample = static_cast<int16_t>(wave * 2700);
            number(sirenPCM, static_cast<uint16_t>(sample), 2);
        }
    }

    void closeSiren() {
        if (!sirenDevice) return;
        waveOutReset(sirenDevice);
        waveOutUnprepareHeader(sirenDevice, &sirenHeader, sizeof(sirenHeader));
        waveOutClose(sirenDevice);
        sirenDevice = nullptr;
        sirenHeader = WAVEHDR{};
    }

    static void number(Bytes& bytes, uint32_t value, int count) {
        for (int i = 0; i < count; ++i)
            bytes.push_back(static_cast<unsigned char>((value >> (i * 8)) & 255));
    }
    static void tag(Bytes& bytes, const char* value) {
        for (int i = 0; i < 4; ++i) bytes.push_back(static_cast<unsigned char>(value[i]));
    }
    void make(Sfx id, initializer_list<Tone> tones) {
        Bytes pcm;
        int ms = 0;
        for (Tone tone : tones) {
            int samples = Rate * tone.ms / 1000;
            ms += tone.ms;
            double phase = 0;
            for (int i = 0; i < samples; ++i) {
                double t = static_cast<double>(i) / max(1, samples - 1);
                double freq = tone.from + (tone.to - tone.from) * t;
                phase += 6.283185307179586 * freq / Rate;
                double envelope = min(1.0, min(i / 100.0, (samples - 1 - i) / 160.0));
                double wave = freq == 0 ? 0 : (0.70 * sin(phase) + 0.30 * sin(3 * phase));
                int16_t sample = static_cast<int16_t>(wave * envelope * 9500);
                number(pcm, static_cast<uint16_t>(sample), 2);
            }
        }
        size_t index = static_cast<size_t>(id);
        Bytes& wav = clips[index];
        tag(wav, "RIFF"); number(wav, static_cast<uint32_t>(36 + pcm.size()), 4);
        tag(wav, "WAVE"); tag(wav, "fmt "); number(wav, 16, 4);
        number(wav, 1, 2); number(wav, 1, 2);
        number(wav, Rate, 4); number(wav, Rate * 2, 4);
        number(wav, 2, 2); number(wav, 16, 2);
        tag(wav, "data"); number(wav, static_cast<uint32_t>(pcm.size()), 4);
        wav.insert(wav.end(), pcm.begin(), pcm.end());
        duration[index] = ms;
    }

public:
    Audio() {
        make(Sfx::Select, { {450, 650, 45} });
        make(Sfx::Start, { {330,330,100},{440,440,100},{587,587,100},{880,990,200} });
        make(Sfx::Pellet, { {950,340,65},{340,550,35} });
        make(Sfx::PelletAlt, { {380,950,65},{950,610,35} });
        make(Sfx::Power, { {260,700,140},{700,1200,180} });
        make(Sfx::Ghost, { {440,1200,130},{1300,1700,85} });
        make(Sfx::Death, { {680,220,370},{220,95,150} });
        make(Sfx::Win, { {523,523,110},{659,659,110},{784,784,110},{1047,1047,320} });
        make(Sfx::Lose, { {392,392,160},{311,311,150},{196,110,360} });
        makeSiren();
        Bytes wav, pcm;
        WAVEFORMATEX format{};
        if (loadWav("sonidos/sirena.wav", wav, pcm, format)) {
            sirenPCM.swap(pcm);
            sirenFormat = format;
        }
        if (loadWav("sonidos/comer.wav", wav, pcm, format)) {
            size_t i = static_cast<size_t>(Sfx::Pellet);
            duration[i] = static_cast<int>(1000ULL * pcm.size() / format.nAvgBytesPerSec);
            clips[i].swap(wav);
            externalPellet = true;
        }
    }
    ~Audio() { stop(); }
    bool isEnabled() const { return enabled; }
    void background(bool active) {
        if (!active || !enabled) { closeSiren(); backgroundAttempted = false; return; }
        if (backgroundAttempted) return;
        backgroundAttempted = true;
        if (waveOutOpen(&sirenDevice, WAVE_MAPPER, &sirenFormat, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
            sirenDevice = nullptr; return;
        }
        sirenHeader.lpData = reinterpret_cast<LPSTR>(sirenPCM.data());
        sirenHeader.dwBufferLength = static_cast<DWORD>(sirenPCM.size());
        sirenHeader.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        sirenHeader.dwLoops = 0xFFFFFFFFu;
        if (waveOutPrepareHeader(sirenDevice, &sirenHeader, sizeof(sirenHeader)) != MMSYSERR_NOERROR) {
            waveOutClose(sirenDevice); sirenDevice = nullptr; sirenHeader = WAVEHDR{}; return;
        }
        if (waveOutWrite(sirenDevice, &sirenHeader, sizeof(sirenHeader)) != MMSYSERR_NOERROR) closeSiren();
    }
    void stop() {
        background(false);
        alternatePellet = false;
        PlaySoundA(nullptr, nullptr, 0);
        until = chrono::steady_clock::time_point{};
        currentPriority = 0;
    }
    void toggle() { enabled = !enabled; if (!enabled) stop(); }
    void play(Sfx id) {
        if (!enabled) return;
        if (id == Sfx::Pellet && !externalPellet) {
            if (alternatePellet) id = Sfx::PelletAlt;
            alternatePellet = !alternatePellet;
        }
        int priority = (id == Sfx::Pellet || id == Sfx::PelletAlt || id == Sfx::Select) ? 0 :
                       (id == Sfx::Win || id == Sfx::Lose || id == Sfx::Death) ? 3 : 1;
        auto now = chrono::steady_clock::now();
        if (now < until && priority < currentPriority) return;
        size_t i = static_cast<size_t>(id);
        if (PlaySoundA(reinterpret_cast<LPCSTR>(clips[i].data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT)) {
            currentPriority = priority;
            until = now + chrono::milliseconds(duration[i]);
        }
    }
};

Audio& audio() { static Audio instance; return instance; }

struct Pos { int x, y; };
bool operator==(Pos a, Pos b) { return a.x == b.x && a.y == b.y; }
Pos operator+(Pos a, Pos b) { return { a.x + b.x, a.y + b.y }; }
const Pos STOP = { 0, 0 };
const array<Pos, 4> DIRS = { {{0, -1}, {-1, 0}, {0, 1}, {1, 0}} };

const array<string, H> ORIGINAL = { {
    "#############################",
    "#o............#............o#",
    "#.####.######.#.######.####.#",
    "#...........................#",
    "#.####.##.#########.##.####.#",
    "#......##.....#.....##......#",
    "######.######.#.######.######",
    "#............ # ............#",
    "#.####.##.###   ###.##.####.#",
    "#......#             #......#",
    "######.# ###     ### #.######",
    "#......#             #......#",
    "#.####.##.###   ###.##.####.#",
    "#............   ............#",
    "######.##.#########.##.######",
    "#......##.....#.....##......#",
    "#.####.######.#.######.####.#",
    "#o...#........ ........#...o#",
    "####.#.##.#########.##.#.####",
    "#...........................#",
    "#############################"
} };

struct Ghost {
    Pos pos, home;
    char symbol;
    int wait;
};

enum class State { Playing, Won, Lost, Exit };

class Game {
public:
    array<string, H> map = ORIGINAL;
    Pos player = { 14, 17 }, direction = STOP, wanted = STOP;
    array<Ghost, 4> ghosts = { {
        {{13, 9},  {13, 9},  'B', 0},
        {{15, 9},  {15, 9},  'P', 0},
        {{13, 11}, {13, 11}, 'I', 0},
        {{15, 11}, {15, 11}, 'D', 0}
    } };
    int score = 0, lives = 3, remaining = 0, power = 0;
    int ticks = 0, combo = 0;
    State state = State::Playing;
    mt19937 rng;

    explicit Game(unsigned seed = random_device{}()) : rng(seed) {
        for (const auto& row : map)
            for (char c : row)
                if (c == '.' || c == 'o') ++remaining;
        resetPositions();
    }

    bool walkable(Pos p) const {
        return p.x >= 0 && p.x < W && p.y >= 0 && p.y < H && map[p.y][p.x] != '#';
    }

    using Dist = array<array<int, W>, H>;
    Dist distances(Pos target) const {
        Dist d;
        for (auto& row : d) row.fill(-1);
        if (!walkable(target)) return d;
        queue<Pos> q;
        q.push(target);
        d[target.y][target.x] = 0;
        while (!q.empty()) {
            Pos p = q.front(); q.pop();
            for (Pos dir : DIRS) {
                Pos next = p + dir;
                if (walkable(next) && d[next.y][next.x] == -1) {
                    d[next.y][next.x] = d[p.y][p.x] + 1;
                    q.push(next);
                }
            }
        }
        return d;
    }

    Pos ahead(Pos dir, int steps) const {
        Pos target = player;
        for (int i = 0; i < steps; ++i) {
            Pos next = target + dir;
            if (!walkable(next)) break;
            target = next;
        }
        return target;
    }

    void moveGhost(int i) {
        Ghost& g = ghosts[i];
        if (g.wait > 0) { --g.wait; return; }
        if (power > 0 ? ticks % 2 == 0 : ticks % 4 == 0) return;

        vector<Pos> options;
        for (Pos dir : DIRS) {
            Pos next = g.pos + dir;
            if (walkable(next)) options.push_back(next);
        }
        if (options.empty()) return;

        if (power == 0 && uniform_int_distribution<int>(0, 99)(rng) < 18) {
            g.pos = options[uniform_int_distribution<int>(0, static_cast<int>(options.size()) - 1)(rng)];
            return;
        }
        Pos target = player;
        if (power == 0) {
            if (i == 1) target = ahead(direction, 4);
            if (i == 2) target = ahead({ -direction.y, direction.x }, 3);
            if (i == 3 && abs(g.pos.x - player.x) + abs(g.pos.y - player.y) < 7)
                target = { 1, 19 };
        }
        Dist d = distances(target);
        shuffle(options.begin(), options.end(), rng);
        Pos best = options.front();
        for (Pos p : options) {
            int candidate = d[p.y][p.x], previous = d[best.y][best.x];
            if (candidate < 0) continue;
            if (previous < 0 || (power > 0 ? candidate > previous : candidate < previous))
                best = p;
        }
        g.pos = best;
    }

    void collect() {
        char& tile = map[player.y][player.x];
        if (tile == '.') { score += 10; --remaining; tile = ' '; audio().play(Sfx::Pellet); }
        else if (tile == 'o') {
            score += 50; --remaining; tile = ' ';
            power = POWER_TIME; combo = 0;
            audio().play(Sfx::Power);
        }
    }

    bool collision(Ghost& g) {
        if (g.wait > 0 || !(g.pos == player)) return false;
        if (power > 0) {
            score += 200 * (1 << min(combo, 3));
            ++combo;
            audio().play(Sfx::Ghost);
            g.pos = g.home;
            g.wait = 18;
            return false;
        }
        --lives;
        if (lives == 0) { state = State::Lost; audio().play(Sfx::Lose); }
        else { audio().play(Sfx::Death); resetPositions(); }
        return true;
    }

    void resetPositions() {
        player = { 14, 17 };
        direction = wanted = STOP;
        power = combo = ticks = 0;
        for (int i = 0; i < 4; ++i) {
            ghosts[i].pos = ghosts[i].home;
            ghosts[i].wait = 8 + i * 5;
        }
    }

    void step() {
        if (state != State::Playing) return;
        ++ticks;
        if (power > 0 && --power == 0) combo = 0;

        if (walkable(player + wanted)) direction = wanted;
        if (walkable(player + direction)) player = player + direction;
        collect();
        if (remaining == 0) { state = State::Won; audio().play(Sfx::Win); return; }

        for (auto& g : ghosts) if (collision(g)) return;
        for (int i = 0; i < 4; ++i) {
            moveGhost(i);
            if (collision(ghosts[i])) return;
        }
    }
};

const int SW = 62, SH = 25;
const WORD BLUE = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
const WORD YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
const WORD GRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
const WORD CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD RED = FOREGROUND_RED | FOREGROUND_INTENSITY;
const WORD PINK = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
const array<WORD, 4> GHOST_COLORS = { {RED, PINK, CYAN, FOREGROUND_RED | FOREGROUND_GREEN} };

class Screen {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO originalCursor{};
    bool hasCursor = false;
    array<CHAR_INFO, SW* SH> buffer{};
public:
    Screen() {
        SetConsoleTitleA("PAC-MAN | C++");
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (GetConsoleScreenBufferInfo(out, &info)) {
            COORD size = { static_cast<SHORT>(max<int>(SW, info.dwSize.X)),
                           static_cast<SHORT>(max<int>(SH, info.dwSize.Y)) };
            SetConsoleScreenBufferSize(out, size);
            COORD largest = GetLargestConsoleWindowSize(out);
            int width = min<int>(size.X, largest.X);
            int height = min<int>(size.Y, largest.Y);
            if (width >= SW && height >= SH) {
                SMALL_RECT window = { 0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1) };
                SetConsoleWindowInfo(out, TRUE, &window);
            }
        }
        hasCursor = GetConsoleCursorInfo(out, &originalCursor) != 0;
        if (hasCursor) {
            CONSOLE_CURSOR_INFO hidden = originalCursor;
            hidden.bVisible = FALSE;
            SetConsoleCursorInfo(out, &hidden);
        }
    }
    ~Screen() {
        if (hasCursor) SetConsoleCursorInfo(out, &originalCursor);
        SetConsoleTextAttribute(out, GRAY);
        SetConsoleCursorPosition(out, { 0, SH - 1 });
        cout << '\n';
    }
    void put(int x, int y, char c, WORD color) {
        if (x < 0 || x >= SW || y < 0 || y >= SH) return;
        auto& cell = buffer[y * SW + x];
        cell.Char.AsciiChar = c;
        cell.Attributes = color;
    }
    void text(int x, int y, const string& s, WORD color) {
        for (char c : s) put(x++, y, c, color);
    }
    void clear() {
        for (auto& cell : buffer) { cell.Char.AsciiChar = ' '; cell.Attributes = GRAY; }
    }
    void present() {
        SMALL_RECT region = { 0, 0, SW - 1, SH - 1 };
        WriteConsoleOutputA(out, buffer.data(), { SW, SH }, { 0, 0 }, &region);
    }
    void centered(int y, const string& line, WORD color) {
        text((SW - static_cast<int>(line.size())) / 2, y, line, color);
    }
    void menu(int selected) {
        clear();
        text(0, 0, "+" + string(SW - 2, '=') + "+", BLUE);
        text(0, SH - 1, "+" + string(SW - 2, '=') + "+", BLUE);
        for (int y = 1; y < SH - 1; ++y) {
            put(0, y, '|', BLUE); put(SW - 1, y, '|', BLUE);
        }
        centered(2, "####   ###   ####       #   #   ###   #   #", YELLOW);
        centered(3, "#   # #   # #           ## ##  #   #  ##  #", YELLOW);
        centered(4, "####  ##### #     ----- # # #  #####  # # #", YELLOW);
        centered(5, "#     #   # #           #   #  #   #  #  ##", YELLOW);
        centered(6, "#     #   #  ####       #   #  #   #  #   #", YELLOW);
        centered(8, "A R C A D E   E D I T I O N", CYAN);
        text(10, 10, "C  .  .  .  .  .", YELLOW);
        for (int i = 0; i < 4; ++i) text(31 + i * 5, 10, "[" + string(1, "BPID"[i]) + "]", GHOST_COLORS[i]);
        centered(13, string(selected == 0 ? ">  " : "   ") + "1. COMENZAR PARTIDA", selected == 0 ? YELLOW : WHITE);
        centered(15, string(selected == 1 ? ">  " : "   ") + "2. SALIR DEL JUEGO ", selected == 1 ? YELLOW : WHITE);
        centered(18, "W/S o flechas: seleccionar | ENTER: confirmar", GRAY);
        centered(19, "Tambien puedes pulsar 1 o 2 directamente", GRAY);
        centered(21, audio().isEnabled() ? "SONIDO: ACTIVADO  [M]" : "SONIDO: DESACTIVADO  [M]", audio().isEnabled() ? GREEN : GRAY);
        centered(23, "C++  |  Consola de Windows", CYAN);
        present();
    }
    void render(const Game& game, bool paused) {
        clear();
        text(2, 0, "P A C - M A N", YELLOW);
        text(22, 0, "Puntos: " + to_string(game.score), WHITE);
        text(45, 0, "Vidas: " + to_string(game.lives), RED);
        text(2, 1, "Restantes: " + to_string(game.remaining), WHITE);
        if (game.power > 0) {
            string bar(10, '.');
            for (int i = 0; i < (game.power * 10 + POWER_TIME - 1) / POWER_TIME; ++i) bar[i] = '=';
            text(30, 1, "PODER [" + bar + "]", CYAN);
        }
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                char c = game.map[y][x];
                int dx = 2 + x * 2, dy = 2 + y;
                if (c == '#') text(dx, dy, "##", BLUE);
                else if (c == '.') put(dx, dy, '.', GRAY);
                else if (c == 'o') put(dx, dy, 'o', YELLOW);
            }
        }
        for (size_t i = 0; i < game.ghosts.size(); ++i) {
            const Ghost& g = game.ghosts[i];
            if (g.wait > 0 && game.ticks % 2 == 0) continue;
            WORD color = game.power > 0 ? (game.power < 15 && game.ticks % 2 == 0 ? WHITE : BLUE) : GHOST_COLORS[i];
            put(2 + g.pos.x * 2, 2 + g.pos.y, game.power > 0 && g.wait == 0 ? 'v' : g.symbol, color);
        }
        char pac = 'C';
        if (game.direction.x < 0) pac = '>';
        if (game.direction.x > 0) pac = '<';
        if (game.direction.y < 0) pac = 'v';
        if (game.direction.y > 0) pac = '^';
        put(2 + game.player.x * 2, 2 + game.player.y, pac, YELLOW);

        if (game.state == State::Won) text(2, 23, "VICTORIA! Recogiste todos los puntos. R: reiniciar", GREEN);
        else if (game.state == State::Lost) text(2, 23, "GAME OVER! Pulsa R para volver a jugar.", RED);
        else if (paused) text(2, 23, "PAUSA - Pulsa P para continuar.", CYAN);
        else text(2, 23, "WASD / Flechas: mover | P: pausa | R: reiniciar", WHITE);
        text(2, 24, audio().isEnabled() ? "ESC: menu | M: silenciar | B/P/I/D: fantasmas" : "ESC: menu | M: activar sonido | B/P/I/D: fantasmas", GRAY);
        present();
    }
};

int readKey() {
    int key = _getch();
    if (key == 0 || key == 224) return 1000 + _getch();
    return key;
}

bool mainMenu(Screen& screen) {
    int selected = 0;
    screen.menu(selected);
    while (true) {
        int key = readKey();
        switch (key) {
        case 1072: case 1080: case 'w': case 'W': case 's': case 'S':
            selected = 1 - selected; audio().play(Sfx::Select); break;
        case '1': return true;
        case '2': case 27: return false;
        case 13: return selected == 0;
        case 'm': case 'M':
            audio().toggle();
            if (audio().isEnabled()) audio().play(Sfx::Select);
            break;
        }
        screen.menu(selected);
    }
}

void playGame(Screen& screen) {
    Game game;
    bool paused = false;
    audio().stop(); audio().play(Sfx::Start);
    screen.render(game, paused);
    while (game.state != State::Exit) {
        auto start = chrono::steady_clock::now();
        while (_kbhit()) {
            switch (readKey()) {
            case 1072: case 'w': case 'W': game.wanted = { 0, -1 }; break;
            case 1080: case 's': case 'S': game.wanted = { 0, 1 }; break;
            case 1075: case 'a': case 'A': game.wanted = { -1, 0 }; break;
            case 1077: case 'd': case 'D': game.wanted = { 1, 0 }; break;
            case 'p': case 'P': if (game.state == State::Playing) paused = !paused; break;
            case 'm': case 'M': audio().toggle(); break;
            case 'r': case 'R':
                game = Game(); paused = false;
                audio().stop(); audio().play(Sfx::Start);
                break;
            case 27: game.state = State::Exit; break;
            }
            if (game.state == State::Exit) break;
        }
        if (game.state == State::Exit) break;
        if (!paused && !(game.wanted == STOP)) game.step();
        audio().background(!paused && game.state == State::Playing && !(game.wanted == STOP));
        screen.render(game, paused);
        this_thread::sleep_until(start + chrono::milliseconds(TICK_MS));
    }
    audio().stop();
}

int main() {
    Screen screen;
    while (mainMenu(screen)) playGame(screen);
    audio().stop();
    screen.clear();
    screen.centered(11, "GRACIAS POR JUGAR PAC-MAN!", YELLOW);
    screen.present();
    this_thread::sleep_for(chrono::milliseconds(550));
    return 0;
}