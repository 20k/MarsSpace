#include <iostream>

#include "planet_gen.h"
#include <SFML/Graphics.hpp>
#include "entities.h"

#include "misc.h"
#include "air.hpp"

using namespace std;


int main()
{
    int width = 1500, height = 800;

    int gen_width = 500;
    int gen_height = 500;

    float view_ratio = (float)height/width;

    sf::RenderWindow win(sf::VideoMode(width, height), "hi");

    sf::View view;
    view.reset(sf::FloatRect(0, 0, gen_width, gen_height));

    planet_gen gen;
    auto tex = gen.get_tex(gen_width, gen_height);

    sf::Sprite spr;
    spr.setTexture(tex);

    air_processor air_process;
    air_process.load(gen_width, gen_height);

    state st(&win, tex, air_process);

    player* play = new player("res/character.png");

    building* build = new building;
    //build->add_wall({0.f, 0.f}, {255.f, 255.f});

    std::vector<entity*> stuff;
    stuff.push_back(new planet(tex));
    stuff.push_back(build);
    stuff.push_back(play);

    play->position = (vec2f){gen_width/2.f - 5, gen_height/2.f};
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

    float zoom_level = 0.55;

    history<vec2f> mouse_clicks;
    history<vec2f> mouse_rclicks;

    saver save;

    sf::Clock clk;

    sf::Mouse mouse;

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

        view.reset(sf::FloatRect(0, 0, gen_width, gen_width * view_ratio));
        view.setCenter(gen_width/2.f, gen_height/2.f);
        view.zoom(zoom_level);

        win.setView(view);

        if(key.isKeyPressed(sf::Keyboard::Escape))
            win.close();

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

        if(once<sf::Keyboard::B>())
        {
            auto vec = stuff;
            vec.push_back(play);

            save.save_to_file("save.txt", vec);
        }

        if(once<sf::Keyboard::N>())
        {
            stuff = save.load_from_file("save.txt", st);
            build->walls.clear();
            stuff.push_back(build);

            play = dynamic_cast<player*>(st.current_player);
        }

        if(mouse.isButtonPressed(sf::Mouse::Middle))
        {
            vec2f local_pos = m_fetch.get_world(st);

            air_process.add(local_pos.v[0], local_pos.v[1], 1.f, air::OXYGEN);
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

            stuff.push_back(new door(m2, m1, 2000.f));
        }


        float dt = clk.getElapsedTime().asMicroseconds() / 1000.f;
        clk.restart();

        air_process.tick(st, dt);

        for(auto& i : stuff)
        {
            i->tick(st, dt);
        }

        vec2f rounded_mouse_pos = round_to_multiple(mouse_pos, 5);

        shape.setPosition(rounded_mouse_pos.v[0], rounded_mouse_pos.v[1]);
        win.draw(shape);

        //play->tick(st, dt);

        air_process.draw(st);

        //mydoor.tick(st, dt);

        //open.tick(dt);

        //win.draw(spr);
        win.display();
        win.clear();

        printf("%f\n", dt);

        //sf::sleep(sf::microseconds(100000.f));

        /*sf::Sprite spr2;
        spr2.setTexture(wtex.getTexture());

        win.draw(spr2);
        win.display();*/
    }

    return 0;
}
