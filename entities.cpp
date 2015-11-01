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

door::door(vec2f _start, vec2f _finish, float time_to_open) :
    open(time_to_open),
    interact((_finish - _start).rot(M_PI/2.f).norm() * 5.f + (_start + _finish)/2.f, 2.f), ///temp
    block(_start, _finish)
{
    fixed_start = _start;
    fixed_finish = _finish;
}

void door::tick(state& s, float dt)
{
    if(interact.player_has_interacted(s))
    {
        open.toggle();
    }

    block.tick(s);
    open.tick(dt);
    interact.tick(s);

    ///solve the door partial open problem later, in the opener class
    float close_frac = 1.f - open.get_open_fraction();

    ///at full open we want the rendering to be maximally long, so 1.f - open frac
    vec2f new_end = squash.get_squashed_end(fixed_start, fixed_finish, close_frac);
    block.modify_bounds(fixed_start, new_end);

    rect.tick(s, fixed_start, new_end, 0.5f);
}
