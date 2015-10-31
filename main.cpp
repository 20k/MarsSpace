#include <iostream>

#include "noise.h"

#include <SFML/Graphics.hpp>

using namespace std;

int main()
{
    int width = 500, height = 500;

    float* noise = pnoise_buf(width, height);

    sf::RenderWindow win(sf::VideoMode(800, 600), "hi");

    sf::Image img;
    img.create(width, height);

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            float val = noise[y*width + x];

            val = clamp(val, 0.f, 1.f);

            ///131 79 42 is mars

            vec3f mars = (vec3f){131, 79, 42};

            vec3f res = mars * val;

            img.setPixel(x, y, sf::Color(res.v[0], res.v[1], res.v[2]));
        }
    }

    sf::Texture tex;
    tex.loadFromImage(img);

    sf::Sprite spr;
    spr.setTexture(tex);

    sf::Event Event;
    sf::Keyboard key;

    while(win.isOpen())
    {
        sf::Clock c;

        while(win.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                win.close();
        }

        if(key.isKeyPressed(sf::Keyboard::Escape))
            win.close();

        win.draw(spr);
        win.display();
    }
    cout << "Hello world!" << endl;
    return 0;
}
