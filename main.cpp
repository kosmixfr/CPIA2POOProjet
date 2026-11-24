#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <chrono>
#include <SFML/Graphics.hpp>

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

class Game : public GameDrawable
{
    static constexpr float CELL = 20.f;

    Grille CurrentGrid;
    Grille NextGrid;

    long long LastUpdateTime = 0;

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
    Game(int cols, int rows)
        : CurrentGrid(cols, rows),
          NextGrid(cols, rows),
          window(sf::VideoMode({static_cast<unsigned>(cols * CELL), static_cast<unsigned>(rows * CELL)}),
                 "Cocasse")
    {
        window.setFramerateLimit(60);
        initialState();
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

        if (currentTime - LastUpdateTime >= 1000)
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

        Grille &current = CurrentGrid;
        Grille &next = NextGrid;
        next.Copy(current);
        for (int i = 0; i < current.getHeight(); ++i)
        {
            for (int j = 0; j < current.getWidth(); ++j)
            {
                Cellule *cell = current.getAt(j, i);
                Cellule *nextCell = next.getAt(j, i);
                cell->update(nextCell);
            }
        }
        current.Copy(next);
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

int main()
{
    Game game(40, 30);
    game.run();
    return 0;
}
