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

    vec2f key_dir = key.tick(1.f).norm();

    float cur_speed = speed.get_speed() * dt * 2; ///temporary hack until i get my shit together

    position = mover.tick(s, position, key_dir, cur_speed);

    //display.tick(s, position, position + (vec2f){10.f, -10.f});
}

///we need to set_active the player when loading
player::player(byte_fetch& fetch, state& s) : entity()
{
    ///temp hack. Should the player just own this?
    ///or should I incorporate it into the save system
    file.load("res/character.png");

    position = fetch.get<vec2f>();
    float sp = fetch.get<float>();

    speed.set_speed(sp);

    ///????
    set_active_player(s);
}

save player::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<float>(speed.get_speed());

    return {entity_type::PLAYER, vec};
}

planet::planet(sf::Texture& tex)
{
    file.load(tex);
}

void planet::tick(state& s, float dt)
{
    file.tick(s, (vec2f){0.f, 0.f});
}

planet::planet(byte_fetch& fetch, state& s)
{
    ///hmm
    file.load(s.planet_tex);
}

save planet::make_save()
{
    return {entity_type::PLANET, byte_vector()};
}

void building::add_wall(state& s, vec2f start, vec2f finish)
{
    wall_segment w(start, finish);

    walls.push_back(w);
}

void building::tick(state& s, float dt)
{
    for(auto& i : walls)
        i.tick(s);
}

building::building(byte_fetch& fetch, state& s)
{
    int32_t num = fetch.get<int32_t>();

    for(int i=0; i<num; i++)
    {
        auto start = fetch.get<vec2f>();
        auto finish = fetch.get<vec2f>();

        add_wall(s, start, finish);
    }
}

save building::make_save()
{
    byte_vector vec;

    vec.push_back<int32_t>(walls.size());

    for(auto& i : walls)
    {
        vec.push_back<vec2f>(i.start);
        vec.push_back<vec2f>(i.finish);
    }

    return {entity_type::BUILDING, vec};
}

door::door(vec2f _start, vec2f _finish, float time_to_open) :
    open(time_to_open),
    i1((_finish - _start).rot(M_PI/2.f).norm() * 5.f + (_start + _finish)/2.f, 2.f), ///temp
    i2((_finish - _start).rot(-M_PI/2.f).norm() * 5.f + (_start + _finish)/2.f, 2.f), ///temp
    block(_start, _finish)
{
    fixed_start = _start;
    fixed_finish = _finish;
}

///this is stupid
door::door(door_fudger fudge) : door(fudge.start, fudge.finish, fudge.tto)
{

}

void door::tick(state& s, float dt)
{
    if(i1.player_has_interacted(s) || i2.player_has_interacted(s))
    {
        open.toggle();
    }

    block.tick(s);
    open.tick(dt);
    i1.tick(s);
    i2.tick(s);

    ///solve the door partial open problem later, in the opener class
    float close_frac = 1.f - open.get_open_fraction();

    ///at full open we want the rendering to be maximally long, so 1.f - open frac
    vec2f new_end = squash.get_squashed_end(fixed_start, fixed_finish, close_frac);
    block.modify_bounds(fixed_start, new_end);

    rect.tick(s, fixed_start, new_end, 0.5f);

    //printf("%f %f\n", fixed_start.v[1], new_end.v[1]);
}

///this is a fudge to make the arguments be evaluated in order
///this is completely stupid
door_fudger fudge(byte_fetch& fetch)
{
    door_fudger f;
    f.start = fetch.get<vec2f>();
    f.finish = fetch.get<vec2f>();
    f.tto = fetch.get<float>();

    return f;
}

///Hmm. I need to figure out this initiualisation stuff properly
///class initialisation like this with a byte fetch isn't going to work forever
///c++11 to the rescue temporarily though
///maybe I can solve all the init problems like this though?
door::door(byte_fetch& fetch) : door(fudge(fetch))
{

}

save door::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(fixed_start);
    vec.push_back<vec2f>(fixed_finish);
    vec.push_back<float>(open.time_duration); ///should we make it strictly have to be part of door?

    return {entity_type::DOOR, vec};
}
