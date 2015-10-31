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

    virtual void tick(state& s, float dt) override;
};

struct planet : entity
{
    renderable_texture file;

    planet(sf::Texture& tex);

    virtual void tick(state& s, float dt) override;
};

#endif // ENTITIES_H_INCLUDED
