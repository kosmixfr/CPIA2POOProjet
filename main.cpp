#include <stdio.h>
#include <iostream>


class Drawable : public virtual Drawable {
public:
    virtual void draw() = 0;
};




class Cellule : public Drawable {
    public:
        virtual bool isAlive() = 0;
    
    
};

class CelluleVivante : public Cellule {

    public: 
        bool isAlive() override {
            return true;
        }

};
class CelluleMorte : public Cellule {

    public :
        bool isAlive() override {
            return false;
        }

};

class Game : public Drawable {

};
class Grille : public Drawable {

};