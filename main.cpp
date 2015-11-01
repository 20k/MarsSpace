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
    //stuff.push_back(play);
    stuff.push_back(build);

    play->position = (vec2f){width/2.f - 5, height/2.f};
    play->set_active_player(st);

    //opener open(2000.f);
    //area_interacter area({width/2.f, height/2.f}, 1.f);
    //door mydoor({width/2.f, height/2.f}, {width/2.f + 5, height/2.f}, 2000.f);


    sf::CircleShape shape;
    shape.setFillColor(sf::Color(100, 255, 100, 128));
    shape.setRadius(1.f);
    shape.setOrigin(1.f, 1.f);

    sf::Event Event;
    sf::Keyboard key;

    float zoom_level = 0.25;

    history<vec2f> mouse_clicks;
    history<vec2f> mouse_rclicks;

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

        /*if(key.isKeyPressed(sf::Keyboard::K))
            open.open();
        if(key.isKeyPressed(sf::Keyboard::L))
            open.close();

        printf("%f\n", open.get_open_fraction());*/

        vec2f mouse_pos = m_fetch.get_world(st);

        if(once<sf::Mouse::Left>())
        {
            mouse_clicks.push_back(mouse_pos);
            mouse_rclicks.clear();
        }

        if(once<sf::Mouse::Right>())
        {
            mouse_rclicks.push_back(mouse_pos);
            mouse_clicks.clear();
        }

        if(mouse_clicks.size() == 2)
        {
            vec2f m1 = mouse_clicks.get(0);
            vec2f m2 = mouse_clicks.get(1);

            m1 = round_to_multiple(m1, 5);
            m2 = round_to_multiple(m2, 5);

            mouse_clicks.clear();

            build->add_wall(st, m2, m1);
        }

        if(mouse_rclicks.size() == 2)
        {
            vec2f m1 = mouse_rclicks.get(0);
            vec2f m2 = mouse_rclicks.get(1);

            m1 = round_to_multiple(m1, 5);
            m2 = round_to_multiple(m2, 5);

            mouse_rclicks.clear();

            stuff.push_back(new door(m1, m2, 2000.f));
        }

        float dt = clk.getElapsedTime().asMicroseconds() / 1000.f;
        clk.restart();

        for(auto& i : stuff)
            i->tick(st, dt);

        vec2f rounded_mouse_pos = round_to_multiple(mouse_pos, 5);

        shape.setPosition(rounded_mouse_pos.v[0], rounded_mouse_pos.v[1]);
        win.draw(shape);

        play->tick(st, dt);

        //mydoor.tick(st, dt);

        //open.tick(dt);

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
