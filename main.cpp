#include <iostream>

#include "planet_gen.h"
#include <SFML/Graphics.hpp>
#include "entities.h"

#include "misc.h"
#include "air.hpp"

#include "sound.h"
#include "game_constants.h"

using namespace std;


int main()
{
    int width = 1500, height = 800;

    int gen_width = 500;
    int gen_height = 500;

    float view_ratio = (float)height/width;

    sf::ContextSettings settings;
    settings.antialiasingLevel = 4;

    sf::RenderWindow win(sf::VideoMode(width, height), "hi", sf::Style::Default, settings);

    //win.setFramerateLimit(120);

    sf::View view;
    view.reset(sf::FloatRect(0, 0, gen_width, gen_height));

    planet_gen gen;
    auto tex = gen.get_tex(gen_width, gen_height);

    float* iron_map = gen.get_iron(gen_width, gen_height);

    sf::Sprite spr;
    spr.setTexture(tex);

    air_processor air_process;
    air_process.load(gen_width, gen_height);

    state st(&win, tex, air_process);

    st.iron_map = iron_map;

    st.dimensions = (vec2f){gen_width, gen_height};

    player* play = new player();

    building* build = new building;
    //build->add_wall({0.f, 0.f}, {255.f, 255.f});

    resource_network default_network;

    entity* glue = new repair_entity();

    //build->add_wall(st, (vec2f){0, 0}, (vec2f){gen_width, gen_height});
    //build->walls.back().sub_segments.clear();

    body_model model;
    float val = model.get_o2_blood_volume_used_atmospheric_ps_litres(air::atmospheric_pressure_pa);

    printf("%f\n", val);

    std::vector<entity*> stuff;
    stuff.push_back(new planet(tex));
    stuff.push_back(build);
    stuff.push_back(play);
    stuff.push_back(glue);

    st.entities = &stuff;

    //wall_segment seg = build->walls.back().split_at_fraction(st, 0.5f);

    //build->walls.push_back();
    //build->walls.push_back(seg.split_at_fraction(st, 0.7f));


    play->position = (vec2f){gen_width/2.f - 5, gen_height/2.f};
    play->set_active_player(st);

    glue->set_position(play->position);


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

    history<int> wall_ids;
    history<vec2f> line_points;
    history<vec2f> mouse_clicks;
    history<vec2f> mouse_rclicks;

    saver save;

    sf::Clock clk;

    mouse_fetcher m_fetch;

    ///convert 1 hydrogen to 1 power
    ///with the way I've done this, i'm an idiot
    ///obvs we want to convert 1 hydrogen to more than 1 power....!!
    /*resource_converter convert;
    convert.set_max_storage({{resource::POWER, 100.f}, {resource::HYDROGEN, 1.f}});
    convert.set_usage_ratio({{resource::HYDROGEN, 1.f}});
    convert.set_output_ratio({{resource::POWER, 1.f}});
    convert.set_amount(1.5f / 1000.f);
    convert.set_efficiency(1.f);

    resource_converter c2;
    c2.set_max_storage({{resource::HYDROGEN, 100.f}, {resource::POWER, 10.f}});
    c2.set_usage_ratio({{resource::POWER, 1.f}});
    c2.set_output_ratio({{resource::HYDROGEN, 1.f}});
    c2.set_amount(2.f);
    c2.set_efficiency(1.5f / 1000.f);

    convert.add({{resource::HYDROGEN, 10.f}});

    resource_network net;
    net.add(&convert);
    net.add(&c2);*/

    music::swap_to_song_type(music::MAINMENU);

    //resource_network net;

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

            if(Event.type == sf::Event::KeyPressed)
            {
                if(Event.key.code == sf::Keyboard::Up)
                {
                    st.current_player->dec_inventory();
                }
                if(Event.key.code == sf::Keyboard::Down)
                {
                    st.current_player->inc_inventory();
                }
            }
        }

        zoom_level = clamp(zoom_level, 0.01f, 2.f);

        view.reset(sf::FloatRect(0, 0, gen_width, gen_width * view_ratio));
        //view.setCenter(gen_width/2.f, gen_height/2.f);
        view.setCenter(play->position.v[0], play->position.v[1]);
        view.zoom(zoom_level);

        win.setView(view);

        if(key.isKeyPressed(sf::Keyboard::Escape))
            win.close();

        vec2f mouse_pos = m_fetch.get_world(st);
        vec2f local_mouse = m_fetch.get_screen(st);
        vec2f round_mouse = round_to_multiple(mouse_pos, 5);

        bool mouse_oob = local_mouse.v[0] < 0 || local_mouse.v[1] < 0 || local_mouse.v[0] >= width-1 || local_mouse.v[1] >= height-1;

        if(once<sf::Mouse::Middle>() && !mouse_oob)
        {
            if(wall_ids.size() == 0)
            {
                vec2f line_point = 0.f;
                float minimum_distance = FLT_MAX;
                int minimum_id = -1;

                for(int i=0; i<(int)build->walls.size(); i++)
                {
                    vec2f start, fn;

                    start = build->walls[i].start;
                    fn = build->walls[i].finish;

                    vec2f dist = point2line_shortest(start, (fn - start), mouse_pos);

                    vec2f avg = (start + fn)/2.f;

                    float rad = (start - fn).length() / 2.f;

                    float extra = std::max((mouse_pos - avg).length() - rad, 0.f);

                    ///also need to restrict to bounds
                    if(dist.length() + extra < minimum_distance && (mouse_pos - avg).length() < rad + 10.f)
                    {
                        minimum_distance = dist.length() + extra;
                        minimum_id = i;
                        line_point = mouse_pos + dist;
                    }
                }

                if(minimum_id != -1)
                {
                    wall_ids.push_back(minimum_id);

                    line_points.push_back(line_point);
                }
            }
            else
            {
                int i1 = wall_ids.get(0);

                vec2f start, fn;

                start = build->walls[i1].start;
                fn = build->walls[i1].finish;

                vec2f dist = point2line_shortest(start, (fn - start), mouse_pos);

                vec2f line_point = mouse_pos + dist;

                line_points.push_back(line_point);

                wall_ids.push_back(i1);
            }

        }

        if(wall_ids.size() == 2)
        {
            int i1 = wall_ids.get(1);

            vec2f lp1 = line_points.get(1);
            vec2f lp2 = line_points.get(0);

            wall_segment wall = build->walls[i1];

            ///badcode alert
            vec2f modified_start = (wall.start - wall.finish).norm() * 20.f + wall.start;

            float d1 = (lp1 - modified_start).length();

            float d3 = (lp2 - modified_start).length();

            ///lp1 further from the start than
            if(d1 > d3)
            {
                std::swap(lp1, lp2);
            }

            ///lp1 closest to start, lp2 further away

            wall_segment s1(wall.start, lp1, game::wall_segment_segment_wall_work), s2(lp2, wall.finish, game::wall_segment_segment_wall_work);

            if(wall.sub_segments.size() == 0)
            {
                s1.sub_segments.clear();
                s2.sub_segments.clear();
            }

            if((lp1 - wall.finish).length() <= (wall.start - wall.finish).length())
                build->walls.push_back(s1);

            if((lp2 - wall.start).length() <= (wall.start - wall.finish).length())
                build->walls.push_back(s2);

            build->walls[i1].destroy(st);
            build->walls.erase(build->walls.begin() + i1);

            wall_ids.clear();
            line_points.clear();
        }

        if(once<sf::Keyboard::Tab>())
        {
            for(auto& i : build->walls)
            {
                i.sub_segments.clear();
            }
        }

        if(once<sf::Mouse::Left>() && !mouse_oob)
        {
            mouse_clicks.push_back(mouse_pos);
            mouse_rclicks.clear();
        }

        if(once<sf::Mouse::Right>() && !mouse_oob)
        {
            mouse_rclicks.push_back(mouse_pos);
            mouse_clicks.clear();
        }

        if(once<sf::Keyboard::Insert>())
        {
            entity* en = new resource_packet(air::IRON);
            en->set_position(mouse_pos);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::B>())
        {
            auto vec = stuff;

            save.save_to_file("save.txt", vec);

            st.air_process->save_to_file("atmo.txt");
        }

        if(once<sf::Keyboard::N>())
        {
            //net.clear();
            stuff = save.load_from_file("save.txt", st);
            build->walls.clear();
            //stuff.push_back(build);

            for(auto& i : stuff)
            {
                auto res = dynamic_cast<resource_entity*>(i);

                ///this is going to cause me problems later
                if(res != nullptr && dynamic_cast<resource_packet*>(i) == nullptr)
                {
                    //res->load(net);
                }

                auto bd = dynamic_cast<building*>(i);

                if(bd != nullptr)
                    build = bd;
            }

            st.air_process->load_from_file("atmo.txt");

            play = dynamic_cast<player*>(st.current_player);
        }

        if(once<sf::Keyboard::F>())
        {
            suit_entity* en = play->drop_suit();

            if(en != nullptr)
                stuff.push_back(en);
        }

        if(once<sf::Keyboard::R>())
        {
            entity* en = play->drop_current();

            if(en != nullptr)
            {
                en->to_unload = false;
                stuff.push_back(en);
            }
        }

        if(key.isKeyPressed(sf::Keyboard::Num1))
        {
            vec2f local_pos = m_fetch.get_world(st);

            air_process.add(local_pos.v[0], local_pos.v[1], 1.f, air::NITROGEN);
        }

        if(key.isKeyPressed(sf::Keyboard::Num2))
        {
            vec2f local_pos = m_fetch.get_world(st);

            air_process.add(local_pos.v[0], local_pos.v[1], 1.f, air::OXYGEN);
        }

        if(key.isKeyPressed(sf::Keyboard::Num3))
        {
            vec2f local_pos = m_fetch.get_world(st);

            air_process.add(local_pos.v[0], local_pos.v[1], 1.f, air::C02);
        }

        st.current_player->set_holding_breath(key.isKeyPressed(sf::Keyboard::Space));

        if(once<sf::Keyboard::F1>())
        {
            solar_panel* en = new solar_panel();
            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F2>())
        {
            hydrogen_battery* en = new hydrogen_battery();
            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F3>())
        {
            gas_storage* en = new gas_storage(air::OXYGEN);
            en->set_position(round_mouse);

            vecrf res = 0.f;

            res.v[air::OXYGEN] = en->conv.max_storage.v[air::OXYGEN] / 2.f;

            en->conv.local_storage = res;

            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F4>())
        {
            gas_storage* en = new gas_storage(air::C02);

            vecrf res = 0.f;

            res.v[air::C02] = 5.f;

            en->conv.local_storage = res;

            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F5>())
        {
            gas_storage* en = new gas_storage(air::NITROGEN);
            en->set_position(round_mouse);

            vecrf res = 0.f;

            res.v[air::NITROGEN] = en->conv.max_storage.v[air::NITROGEN] / 2.f;

            en->conv.local_storage = res;

            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F6>())
        {
            oxygen_reclaimer* en = new oxygen_reclaimer();
            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F7>())
        {
            entity* en = new environment_balancer();
            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F8>())
        {
            entity* en = new resource_filler();
            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F9>())
        {
            entity* en = new resource_network_entity();
            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(once<sf::Keyboard::F10>())
        {
            entity* en = new mining_drill();
            en->set_position(round_mouse);
            stuff.push_back(en);
        }

        if(mouse_clicks.size() == 2)
        {
            vec2f m1 = mouse_clicks.get(0);
            vec2f m2 = mouse_clicks.get(1);

            m1 = round_to_multiple(m1, 5);
            m2 = round_to_multiple(m2, 5);

            mouse_clicks.clear();

            build->add_wall(st, m2, m1, game::wall_segment_segment_wall_work);
        }

        if(mouse_rclicks.size() == 2)
        {
            vec2f m1 = mouse_rclicks.get(0);
            vec2f m2 = mouse_rclicks.get(1);

            m1 = round_to_multiple(m1, 5);
            m2 = round_to_multiple(m2, 5);

            mouse_rclicks.clear();

            stuff.push_back(new door(m2, m1, 2.f));
        }

        ///stuff is always the correct storage
        ///wait. Do i need to even do this then
        st.entities = &stuff;

        float dt = (clk.getElapsedTime().asMicroseconds() / 1000.f) / 1000.f;
        clk.restart();

        air_process.tick(st, dt);

        //for(auto& i : stuff)
        for(int i=0; i<(int)stuff.size(); i++)
        {
            entity* en = stuff[i];

            ///if any previous element has made me invalid
            if(en->to_unload)
            {
                en->to_unload = false;

                if(en->to_delete)
                    delete en;

                stuff.erase(stuff.begin() + i);
                i--;
                continue;
            }

            en->tick(st, dt);

            ///if my tick made me invalid
            if(en->to_unload)
            {
                en->to_unload = false;

                if(en->to_delete)
                    delete en;

                stuff.erase(stuff.begin() + i);
                i--;
                continue;
            }
        }

        //convert.convert(dt / 1000.f);
        //c2.convert(dt / 1000.f);
        /*printf("1 %f %f\n", convert.local_storage.v[resource::HYDROGEN], convert.local_storage.v[resource::POWER]);
        printf("2 %f %f\n\n", c2.local_storage.v[resource::HYDROGEN], c2.local_storage.v[resource::POWER]);
        net.tick(dt / 1000.f);*/

        vec2f rounded_mouse_pos = round_to_multiple(mouse_pos, 5);

        shape.setPosition(rounded_mouse_pos.v[0], rounded_mouse_pos.v[1]);
        win.draw(shape);

        //play->tick(st, dt);

        air_process.draw(st);

        text txt;
        txt.render(st, music::get_current_song_name(), (vec2f){width - 400.f, 10.f}, 10, text_options::ABS);

        win.display();
        win.clear();

        music::tick();

        if(key.isKeyPressed(sf::Keyboard::V))
            printf("%f\n", dt);

        if(key.isKeyPressed(sf::Keyboard::X))
        {
            vec2f clamped_mouse = clamp(mouse_pos, (vec2i){0, 0}, (vec2i){gen_width-1, gen_height-1});

            printf("%f\n", iron_map[(int)clamped_mouse.v[1]*gen_width + (int)clamped_mouse.v[0]]);
        }

        //sf::sleep(sf::microseconds(100000.f));

        /*sf::Sprite spr2;
        spr2.setTexture(wtex.getTexture());

        win.draw(spr2);
        win.display();*/
    }

    music::swap_to_song_type(music::NONE);

    return 0;
}
