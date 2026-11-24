#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>

#include <SFML/Graphics.hpp>



class GameDrawable {
public:
    virtual ~GameDrawable() = default;
    // Dessine l'objet dans `target`, en (x, y), sur une zone de `size` pixels
    virtual void draw(sf::RenderTarget& target, float x, float y, float size) = 0;
};




class Grille;

class Cellule : public GameDrawable {
    public:
        virtual bool isAlive() = 0;
        virtual void MakeAlive() = 0;
        virtual void MakeDead() = 0;
        Cellule(int x, int y, Grille* parent) : x(x), y(y), parentGrid(parent) {}
    private:
        int x, y; // Position in the grid
        Grille* parentGrid; // Pointer to the parent grid, if needed for neighbor checks

    
};

class CelluleVivante : public Cellule {

    public: 
        using Cellule::Cellule;
        bool isAlive() override {
            return true;
        }
        void MakeAlive() override {
            // Already alive, do nothing
        }
        void MakeDead() override {
            // Change state to dead
        }
        void draw(sf::RenderTarget& target, float x, float y, float size) override {
            sf::RectangleShape rect({size - 1.f, size - 1.f});
            rect.setPosition({x, y});
            rect.setFillColor(sf::Color::White);
            target.draw(rect);
        }
        

};
class CelluleMorte : public Cellule {

    public :
        using Cellule::Cellule;
        bool isAlive() override {
            return false;
        }
        void MakeAlive() override {
            // Change state to alive
        }
        void MakeDead() override {
            // Already dead, do nothing
        }
        void draw(sf::RenderTarget& target, float x, float y, float size) override {
            sf::RectangleShape rect({size - 1.f, size - 1.f});
            rect.setPosition({x, y});
            rect.setFillColor(sf::Color(30, 30, 30));
            target.draw(rect);
        }

};

class Grille : public GameDrawable {

    int width;
    int height;
    std::vector<std::vector<Cellule*>> grid;


    void initializeGrid() {
        grid.resize(height, std::vector<Cellule*>(width, nullptr));
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                grid[i][j] = new CelluleMorte(j, i, this); // Initialize all cells as dead
            }
        }
    }

    public:
        Grille(int w, int h) : width(w), height(h) { initializeGrid(); }
        ~Grille() {
            for (auto& row : grid)
                for (Cellule* c : row)
                    delete c;
        }
        Grille(const Grille&) = delete;
        Grille& operator=(const Grille&) = delete;

        // x, y : coin haut-gauche de la grille ; size : taille d'une cellule en pixels
        void draw(sf::RenderTarget& target, float x, float y, float size) override {
            for (int i = 0; i < height; ++i)
                for (int j = 0; j < width; ++j)
                    grid[i][j]->draw(target, x + j * size, y + i * size, size);
        }
        Cellule* getAt(int x, int y) {
            if (x < 0 || x >= width || y < 0 || y >= height)
                return nullptr;
            return grid[y][x];
        }
};

class Game : public GameDrawable {
    static constexpr float CELL = 20.f;

    Grille grille;
    sf::RenderWindow window;

    void handleEvents() {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }
    }

    public:
        Game(int cols, int rows)
            : grille(cols, rows),
              window(sf::VideoMode({static_cast<unsigned>(cols * CELL), static_cast<unsigned>(rows * CELL)}),
                     "Cocasse") {
            window.setFramerateLimit(60);
        }

        void draw(sf::RenderTarget& target, float x, float y, float size) override {
            grille.draw(target, x, y, size);
        }

        void run() {
            while (window.isOpen()) {
                handleEvents();
                window.clear(sf::Color::Black);
                draw(window, 0.f, 0.f, CELL);
                window.display();
            }
        }
};

int main() {
    Game game(40, 30);
    game.run();
    return 0;
}
