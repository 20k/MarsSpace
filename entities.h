#ifndef ENTITIES_H_INCLUDED
#define ENTITIES_H_INCLUDED

#include <vec/vec.hpp>
#include "components.h"

struct entity
{
    vec2f position;

    entity();

    virtual void tick(state& s, float dt){};
};

struct player : entity
{
    renderable_file file;
    moveable mover;
    keyboard_controller key;
    speed_handler speed;

    player(const std::string& fname);

    void set_active_player(state& s);

    virtual void tick(state& s, float dt) override;
};

struct planet : entity
{
    renderable_texture file;

    planet(sf::Texture& tex);

    virtual void tick(state& s, float dt) override;
};

struct building : entity
{
    std::vector<wall_segment> walls;

    void add_wall(state& s, vec2f start, vec2f finish);

    virtual void tick(state& s, float dt) override;
};

/*
///we really want a door component first actually
struct airlock : entity
{
    ///its a kind of building. Maybe structure is a better term
    building build;


};*/

#endif // ENTITIES_H_INCLUDED
