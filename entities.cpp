#include "entities.h"

entity::entity()
{
    position = (vec2f){0.f, 0.f};
}

player::player(const std::string& fname) : entity()
{
    file.load(fname);

    ///should I define units right off the bat
    speed.set_speed(0.014f);
}

void player::set_active_player(state& s)
{
    s.current_player = this;
}

void player::tick(state& s, float dt)
{
    file.tick(s, position);

    vec2f key_dir = key.tick(dt).norm();

    float cur_speed = speed.get_speed();

    position = mover.tick(s, position, key_dir, cur_speed);
}

planet::planet(sf::Texture& tex)
{
    file.load(tex);
}

void planet::tick(state& s, float dt)
{
    file.tick(s, (vec2f){0.f, 0.f});
}

void building::add_wall(state& s, vec2f start, vec2f finish)
{
    wall_segment w(start, finish);

    walls.push_back(w);

    //walls.emplace_back(s, start, finish);
}

void building::tick(state& s, float dt)
{
    for(auto& i : walls)
        i.tick(s);
}
