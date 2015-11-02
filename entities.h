#ifndef ENTITIES_H_INCLUDED
#define ENTITIES_H_INCLUDED

#include <vec/vec.hpp>
#include "components.h"
#include "byte_struct.h"

namespace entity_type
{
    enum entity_type : int32_t
    {
        PLAYER = 0,
        PLANET,
        BUILDING,
        DOOR,
        RESOURCE_ENTITY,
        SOLAR_PANEL,
        HYDROGEN_BATTERY
    };
}

typedef entity_type::entity_type entity_t;

struct save
{
    entity_t type;
    byte_vector vec;
};

struct entity
{
    vec2f position;

    entity();

    virtual void tick(state& s, float dt){};

    virtual save make_save() = 0;
};


struct player : entity
{
    renderable_file file;
    moveable mover;
    keyboard_controller key;
    speed_handler speed;
    air_displayer display; ///generic air unit displayer
    air_monitor monitor; ///environmental monitor
    breather breath;

    player(const std::string& fname);
    player(byte_fetch& fetch, state& s);

    void set_active_player(state& s);

    virtual void tick(state& s, float dt) override;

    save make_save() override;
};

struct planet : entity
{
    renderable_texture file;

    planet(sf::Texture& tex);
    planet(byte_fetch& fetch, state& s);

    virtual void tick(state& s, float dt) override;

    save make_save() override;
};

struct building : entity
{
    std::vector<wall_segment> walls;

    void add_wall(state& s, vec2f start, vec2f finish);

    building() = default;
    building(byte_fetch& fetch, state& s);

    virtual void tick(state& s, float dt) override;

    save make_save() override;
};

struct door_fudger
{
    vec2f start;
    vec2f finish;
    float tto;
};

struct door : entity
{
    squasher squash;
    opener open;
    renderable_rectangle rect;
    area_interacter i1, i2;
    movement_blocker block;
    ///we need a blocker too

    vec2f fixed_start;
    vec2f fixed_finish;

    door(vec2f _start, vec2f _finish, float time_to_open);
    door(door_fudger fudge);
    door(byte_fetch& fetch);

    virtual void tick(state& s, float dt);

    save make_save() override;
};

///we need an object that can pull from the environment and insert into the resource network

///define resource entity
struct resource_entity : entity
{
    resource_converter conv;

    resource_entity(resource_network& net);

    void set_position(vec2f pos);

    virtual void tick(state& s, float dt) override;

    save make_save();
};

struct solar_panel : resource_entity
{
    renderable_file file;

    solar_panel(resource_network& net);

    virtual void tick(state& s, float dt) override;

    save make_save();
};

struct hydrogen_battery : resource_entity
{
    renderable_circle circle;

    hydrogen_battery(resource_network& net);

    virtual void tick(state& s, float dt) override;

    save make_save();
};

/*struct gas_storage : resource_entity
{
    gas_storage(resource_network& net, air_t type);

    virtual void tick(state& s, float dt) override;
};

struct oxygen_reclaimer : resource_entity
{
    oxygen_reclaimer(resource_network& net);

    virtual void tick(state& s, float dt) override;
};*/

///later make resource_network a physical hub or something perhaps?


/*
///we really want a door component first actually
struct airlock : entity
{
    ///its a kind of building. Maybe structure is a better term
    building build;


};*/

#endif // ENTITIES_H_INCLUDED
