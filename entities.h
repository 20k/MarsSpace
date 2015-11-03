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
        HYDROGEN_BATTERY,
        GAS_STORAGE,
        OXYGEN_RECLAIMER,
        SUIT_ENTITY
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
    float rotation;

    entity();

    virtual void tick(state& s, float dt){};

    virtual save make_save() = 0;

    virtual ~entity();

    ///remove me at the next available opportunity
    void schedule_unload();

    bool to_unload;
};

struct suit_entity;

struct player : entity
{
    renderable_file file;
    moveable mover;
    keyboard_controller key;
    speed_handler speed;
    air_displayer display; ///generic air unit displayer
    air_monitor monitor; ///environmental monitor
    breather breath;
    suit mysuit; ///suit needs to be a carried entity soon :[

    player();
    player(byte_fetch& fetch, state& s);

    void set_active_player(state& s);

    virtual void tick(state& s, float dt) override;

    save make_save() override;

    void set_suit(suit& s);
    suit_entity* drop_suit();

private:
    void remove_suit(); ///internal, does not take it off, just stops the player from using it
    bool has_suit;
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
    resource_displayer display;

    resource_converter conv;

    resource_entity();
    resource_entity(resource_network& net);
    resource_entity(byte_fetch& fetch);

    void load(byte_fetch& fetch);
    void load(resource_network& net);

    void set_position(vec2f pos);

    virtual void tick(state& s, float dt) override;

    virtual save make_save();
};

struct solar_panel : resource_entity
{
    renderable_file file;

    solar_panel();
    solar_panel(resource_network& net);
    solar_panel(byte_fetch& fetch);

    virtual void tick(state& s, float dt) override;

    save make_save();
};

struct hydrogen_battery : resource_entity
{
    text txt;
    renderable_circle circle;

    hydrogen_battery();
    hydrogen_battery(resource_network& net);
    hydrogen_battery(byte_fetch& fetch);

    virtual void tick(state& s, float dt) override;

    save make_save();
};

struct gas_storage : resource_entity
{
    renderable_circle circle;
    text txt;
    air_t type;

    gas_storage(air_t type);
    gas_storage(resource_network& net, air_t type);
    gas_storage(byte_fetch& fetch);

    virtual void tick(state& s, float dt) override;

    save make_save();
};

struct oxygen_reclaimer : resource_entity
{
    air_displayer display;

    renderable_circle circle;

    oxygen_reclaimer();
    oxygen_reclaimer(resource_network& _net);
    oxygen_reclaimer(byte_fetch& fetch);

    virtual void tick(state& s, float dt) override;

    save make_save();
};

struct suit_entity : entity
{
    suit this_suit;
    renderable_file file;
    area_interacter interact;

    suit_entity();
    suit_entity(vec2f _pos);

    virtual void tick(state& s, float dt);

    void set_position(vec2f _pos);

    virtual save make_save();
};

///later make resource_network a physical hub or something perhaps?


/*
///we really want a door component first actually
struct airlock : entity
{
    ///its a kind of building. Maybe structure is a better term
    building build;


};*/

#endif // ENTITIES_H_INCLUDED
