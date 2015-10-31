#include <iostream>

#include "planet_gen.h"
#include <SFML/Graphics.hpp>
#include "entities.h"

#include "misc.h"

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

    building* build = new building;
    //build->add_wall({0.f, 0.f}, {255.f, 255.f});

    std::vector<entity*> stuff;
    stuff.push_back(new planet(tex));
    stuff.push_back(play);
    stuff.push_back(build);

    play->position = (vec2f){width/2.f, height/2.f};


    sf::Event Event;
    sf::Keyboard key;

    float zoom_level = 0.25;

    history<vec2f> mouse_clicks;

    sf::Clock clk;

    mouse_fetcher m_fetch;

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

        zoom_level = clamp(zoom_level, 0.01f, 2.f);

        view.reset(sf::FloatRect(0, 0, width, height));
        view.zoom(zoom_level);

        win.setView(view);

        if(key.isKeyPressed(sf::Keyboard::Escape))
            win.close();

        vec2f mouse_pos = m_fetch.get(st);

        if(once<sf::Mouse::Left>())
        {
            mouse_clicks.push_back(mouse_pos);
        }

        if(mouse_clicks.size() == 2)
        {
            vec2f m1 = mouse_clicks.get(0);
            vec2f m2 = mouse_clicks.get(1);

            mouse_clicks.clear();

            build->add_wall(st, m2, m1);
        }

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
