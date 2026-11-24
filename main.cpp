#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <SFML/Graphics.hpp>

// Nombre maximal d'itérations (générations) de la simulation
const int max_iter = 1000;

class GameDrawable
{
public:
    virtual ~GameDrawable() = default;
    // Dessine l'objet dans `target`, en (x, y), sur une zone de `size` pixels
    virtual void draw(sf::RenderTarget &target, float x, float y, float size) = 0;
};

class Grille;

class Cellule : public GameDrawable
{
public:
    virtual bool isAlive() = 0;
    virtual void MakeAlive() = 0;
    virtual void MakeDead() = 0;
    Cellule(int x, int y, Grille *parent) : x(x), y(y), parentGrid(parent) {}

    void update(Cellule *nextState)
    {
        // Implementation for updating cell state
        std::vector<Cellule *> neighbors = getNeighbors();
        int aliveNeighbors = std::count_if(neighbors.begin(), neighbors.end(), [](Cellule *c)
                                           { return c->isAlive(); });
        switch (aliveNeighbors)
        {
        case 2:
            if (isAlive())
                nextState->MakeAlive();
            else
                nextState->MakeDead();
            break;
        case 3:
            nextState->MakeAlive();
            break;
        default:
            nextState->MakeDead();
            break;
        }

    }

    std::vector<Cellule *> getNeighbors(); // défini après Grille

protected:
    int x, y;           // Position in the grid
    Grille *parentGrid; // Pointer to the parent grid
};

class Grille : public GameDrawable
{

    int width;
    int height;
    std::vector<std::vector<Cellule *>> Grid;

    void initializeGrid(); // défini après les cellules

public:
    Grille(int w, int h) : width(w), height(h) { initializeGrid(); }
    ~Grille()
    {
        for (auto &row : Grid)
            for (Cellule *c : row)
                delete c;
    }
    Grille(const Grille &) = delete;
    Grille &operator=(const Grille &) = delete;

    // x, y : coin haut-gauche de la grille ; size : taille d'une cellule en pixels
    void draw(sf::RenderTarget &target, float x, float y, float size) override
    {
        for (int i = 0; i < height; ++i)
            for (int j = 0; j < width; ++j)
                Grid[i][j]->draw(target, x + j * size, y + i * size, size);
    }
    Cellule *getAt(int x, int y)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return nullptr;
        return Grid[y][x];
    }
    // Remplace la cellule en (x, y) par une vivante/morte (si l'état change)
    void setAlive(int x, int y, bool alive);
    void Copy(Grille &other)
    {
        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                if (other.getAt(j, i)->isAlive())
                    Grid[i][j]->MakeAlive();
                else
                    Grid[i][j]->MakeDead();
            }
        }
    }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

std::vector<Cellule *> Cellule::getNeighbors()
{
    std::vector<Cellule *> neighbors;
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            if (dx == 0 && dy == 0)
                continue; // Skip the cell itself
            Cellule *neighbor = parentGrid->getAt(x + dx, y + dy);
            if (neighbor)
                neighbors.push_back(neighbor);
        }
    }
    return neighbors;
}

class CelluleVivante : public Cellule
{
public:
    using Cellule::Cellule;
    bool isAlive() override { return true; }
    void MakeAlive() override
    {
        // Already alive, do nothing
    }
    void MakeDead() override { parentGrid->setAlive(x, y, false); }
    void draw(sf::RenderTarget &target, float x, float y, float size) override
    {
        sf::RectangleShape rect({size - 1.f, size - 1.f});
        rect.setPosition({x, y});
        rect.setFillColor(sf::Color::White);
        target.draw(rect);
    }
};

class CelluleMorte : public Cellule
{
public:
    using Cellule::Cellule;
    bool isAlive() override { return false; }
    void MakeAlive() override { parentGrid->setAlive(x, y, true); }
    void MakeDead() override
    {
        // Already dead, do nothing
    }
    void draw(sf::RenderTarget &target, float x, float y, float size) override
    {
        sf::RectangleShape rect({size - 1.f, size - 1.f});
        rect.setPosition({x, y});
        rect.setFillColor(sf::Color(30, 30, 30));
        target.draw(rect);
    }
};

void Grille::initializeGrid()
{
    Grid.resize(height, std::vector<Cellule *>(width, nullptr));
    for (int i = 0; i < height; ++i)
        for (int j = 0; j < width; ++j)
            Grid[i][j] = new CelluleMorte(j, i, this); // Toutes les cellules mortes au départ
}

void Grille::setAlive(int x, int y, bool alive)
{
    Cellule *old = getAt(x, y);
    if (!old || old->isAlive() == alive)
        return;
    if (alive)
        Grid[y][x] = new CelluleVivante(x, y, this);
    else
        Grid[y][x] = new CelluleMorte(x, y, this);
    delete old;
}

// Charge un état initial depuis un CSV : une ligne par ligne de la grille,
// valeurs séparées par des virgules, 1 = vivante, 0 = morte.
// Les valeurs hors de la grille sont ignorées. Retourne false si le fichier n'a pas pu être ouvert.
// Charge un état depuis un flux CSV (une ligne par ligne de grille, 1 = vivante, 0 = morte)
bool load_grille_stream(Grille &grille, std::istream &in)
{
    std::string line;
    int y = 0;
    while (y < grille.getHeight() && std::getline(in, line))
    {
        std::istringstream lineStream(line);
        std::string token;
        int x = 0;
        while (x < grille.getWidth() && std::getline(lineStream, token, ','))
        {
            int value = 0; // reste à 0 si le champ n'est pas un nombre
            std::istringstream(token) >> value;
            grille.setAlive(x, y, value != 0);
            ++x;
        }
        ++y;
    }
    return true;
}

bool load_grille(Grille &grille, const std::string &path)
{
    std::ifstream file(path);
    if (!file)
        return false;
    return load_grille_stream(grille, file);
}

// Écrit l'état de la grille au format CSV (1 = vivante, 0 = morte)
void save_grille(Grille &grille, std::ostream &out)
{
    for (int y = 0; y < grille.getHeight(); ++y)
    {
        for (int x = 0; x < grille.getWidth(); ++x)
        {
            if (x > 0)
                out << ',';
            out << (grille.getAt(x, y)->isAlive() ? 1 : 0);
        }
        out << '\n';
    }
}

// Calcule une itération : `current` reçoit l'état suivant (`next` sert de tampon).
// Retourne false si aucune cellule n'a changé (automate stable).
bool step(Grille &current, Grille &next)
{
    bool changed = false;
    next.Copy(current);
    for (int y = 0; y < current.getHeight(); ++y)
    {
        for (int x = 0; x < current.getWidth(); ++x)
        {
            bool before = current.getAt(x, y)->isAlive();
            current.getAt(x, y)->update(next.getAt(x, y));
            if (next.getAt(x, y)->isAlive() != before)
                changed = true;
        }
    }
    current.Copy(next);
    return changed;
}

// Vrai si les deux grilles ont exactement le même état
bool same_state(Grille &a, Grille &b)
{
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
        return false;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
            if (a.getAt(x, y)->isAlive() != b.getAt(x, y)->isAlive())
                return false;
    return true;
}

// Mode console : écrit les `iterations` premières générations dans <inputPath>_out/iter_<k>.csv.
// S'arrête plus tôt si l'automate n'évolue plus.
void run_console(Grille &grille, const std::string &inputPath, int iterations)
{
    const std::filesystem::path dir = inputPath + "_out";
    std::filesystem::create_directories(dir);
    Grille next(grille.getWidth(), grille.getHeight());
    for (int k = 1; k <= iterations; ++k)
    {
        bool changed = step(grille, next);
        std::ofstream out(dir / ("iter_" + std::to_string(k) + ".csv"));
        save_grille(grille, out);
        if (!changed)
            break;
    }
}

// Tests unitaires : `./cocasse --test`
using Cells = std::vector<std::pair<int, int>>; // cellules vivantes (x, y)

std::string csv_from_cells(int w, int h, const Cells &alive)
{
    std::string s;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            if (x > 0)
                s += ',';
            bool isAlive = std::find(alive.begin(), alive.end(), std::make_pair(x, y)) != alive.end();
            s += isAlive ? '1' : '0';
        }
        s += '\n';
    }
    return s;
}

// Simule `iterations` générations depuis `initial` et vérifie que la grille égale `expected`
bool check_after_iterations(int w, int h, const Cells &initial, const Cells &expected, int iterations)
{
    Grille current(w, h), next(w, h), want(w, h);
    std::istringstream initIn(csv_from_cells(w, h, initial)), expIn(csv_from_cells(w, h, expected));
    load_grille_stream(current, initIn);
    load_grille_stream(want, expIn);
    for (int k = 0; k < iterations; ++k)
        step(current, next);
    return same_state(current, want);
}

bool run_tests()
{
    struct Case
    {
        std::string name;
        int w, h;
        Cells initial, expected;
        int iterations;
    };
    const Cells blinkerH = {{1, 2}, {2, 2}, {3, 2}};
    const Cells blinkerV = {{2, 1}, {2, 2}, {2, 3}};
    const std::vector<Case> cases = {
        {"cellule seule meurt", 3, 3, {{1, 1}}, {}, 1},
        {"blinker : 1 itération -> vertical", 5, 5, blinkerH, blinkerV, 1},
        {"blinker : 2 itérations -> horizontal", 5, 5, blinkerH, blinkerH, 2},
        {"bloc : stable", 4, 4, {{1, 1}, {2, 1}, {1, 2}, {2, 2}}, {{1, 1}, {2, 1}, {1, 2}, {2, 2}}, 5},
        {"planeur : 4 itérations -> décalé de (1,1)", 10, 10,
         {{3, 2}, {4, 3}, {2, 4}, {3, 4}, {4, 4}},
         {{4, 3}, {5, 4}, {3, 5}, {4, 5}, {5, 5}}, 4},
    };

    int failures = 0;
    for (const Case &c : cases)
    {
        bool ok = check_after_iterations(c.w, c.h, c.initial, c.expected, c.iterations);
        std::cout << (ok ? "[OK]   " : "[FAIL] ") << c.name << std::endl;
        if (!ok)
            ++failures;
    }
    std::cout << (cases.size() - failures) << "/" << cases.size() << " tests réussis" << std::endl;
    return failures == 0;
}

class Game : public GameDrawable
{
    static constexpr float CELL = 20.f;

    Grille CurrentGrid;
    Grille NextGrid;

    long long LastUpdateTime = 0;
    int hz = 100000; // Update frequency in Hz

    sf::RenderWindow window;

    void handleEvents()
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto *key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }
    }

public:
    // path : CSV d'état initial ; vide => état initial par défaut
    Game(int cols, int rows, const std::string &path = "")
        : CurrentGrid(cols, rows),
          NextGrid(cols, rows),
          window(sf::VideoMode({static_cast<unsigned>(cols * CELL), static_cast<unsigned>(rows * CELL)}),
                 "Cocasse")
    {
        window.setFramerateLimit(60);
        if (path.empty())
            initialState();
        else if (!load_grille(CurrentGrid, path))
        {
            std::cerr << "Impossible d'ouvrir " << path << ", état initial par défaut utilisé" << std::endl;
            initialState();
        }
    }

    // État initial : un planeur (glider) et un R-pentomino au centre
    void initialState()
    {
        const int glider[][2] = {{1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2}};
        for (auto &p : glider)
            CurrentGrid.setAlive(2 + p[0], 2 + p[1], true);

        const int rPentomino[][2] = {{1, 0}, {2, 0}, {0, 1}, {1, 1}, {1, 2}};
        int cx = CurrentGrid.getWidth() / 2, cy = CurrentGrid.getHeight() / 2;
        for (auto &p : rPentomino)
            CurrentGrid.setAlive(cx + p[0], cy + p[1], true);
    }

    void draw(sf::RenderTarget &target, float x, float y, float size) override
    {
        CurrentGrid.draw(target, x, y, size);
    }
    void shouldUpdate()
    {
        long long currentTime = static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                           std::chrono::steady_clock::now().time_since_epoch())
                                                           .count());

        if (currentTime - LastUpdateTime >= 1000 / hz)
        { // Update every second
            LastUpdateTime = currentTime;
            this->update();
            // Here you would implement the logic to update the grid state
            // For now, we will just print a message
            std::cout << "Updating grid..." << std::endl;
        }
    }

    void update()
    {
        step(CurrentGrid, NextGrid);
    }

    void run()
    {
        while (window.isOpen())
        {
            handleEvents();
            shouldUpdate();
            window.clear(sf::Color::Black);
            draw(window, 0.f, 0.f, CELL);
            window.display();
        }
    }
};



// Usage :
//   ./cocasse [fichier.csv]                 mode graphique
//   ./cocasse fichier.csv --console N       mode console (N générations dans fichier.csv_out/)
//   ./cocasse --test                        tests unitaires
int main(int argc, char *argv[])
{
    std::vector<std::string> args(argv + 1, argv + argc);
    if (!args.empty() && args[0] == "--test")
        return run_tests() ? 0 : 1;

    std::string path;
    int consoleIterations = 0;
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        if (args[i] == "--console" && i + 1 < args.size())
            consoleIterations = std::stoi(args[++i]);
        else
            path = args[i];
    }

    if (consoleIterations > 0)
    {
        if (path.empty())
        {
            std::cerr << "Mode console : fichier d'entrée requis" << std::endl;
            return 1;
        }
        Grille grille(100, 100);
        if (!load_grille(grille, path))
        {
            std::cerr << "Impossible d'ouvrir " << path << std::endl;
            return 1;
        }
        run_console(grille, path, consoleIterations);
        return 0;
    }

    Game game(100, 100, path);
    game.run();
    return 0;
}
