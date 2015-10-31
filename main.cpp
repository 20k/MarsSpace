#include <iostream>

#include "planet_gen.h"
#include <SFML/Graphics.hpp>
#include "entities.h"

using namespace std;

int main()
{
    int width = 1500, height = 800;

    sf::RenderWindow win(sf::VideoMode(width, height), "hi");

    sf::View view;
    view.reset(sf::FloatRect(0, 0, width, height));

    planet_gen gen;
    auto tex = gen.get_tex(width, height);

    sf::Sprite spr;
    spr.setTexture(tex);

    state st(&win);

    player* play = new player("res/character.png");

    std::vector<entity*> stuff;
    stuff.push_back(new planet(tex));
    stuff.push_back(play);

    play->position = (vec2f){width/2.f, height/2.f};


    sf::Event Event;
    sf::Keyboard key;

    float zoom_level = 0.25;

    sf::Clock clk;

    while(win.isOpen())
    {
        while(win.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                win.close();

            if(Event.type == sf::Event::MouseWheelScrolled)
            {
                float delta = Event.mouseWheelScroll.delta;

                zoom_level -= zoom_level * delta / 10.f;
            }
        }

        printf("%f\n", zoom_level);

        zoom_level = clamp(zoom_level, 0.01f, 2.f);

        view.reset(sf::FloatRect(0, 0, width, height));
        view.zoom(zoom_level);

        win.setView(view);

        if(key.isKeyPressed(sf::Keyboard::Escape))
            win.close();

        float dt = clk.getElapsedTime().asMicroseconds() / 1000.f;
        clk.restart();

        for(auto& i : stuff)
            i->tick(st, dt);

        //win.draw(spr);
        win.display();
        win.clear();

        /*sf::Sprite spr2;
        spr2.setTexture(wtex.getTexture());

        win.draw(spr2);
        win.display();*/
    }

    return 0;
}
