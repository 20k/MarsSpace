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
        SUIT_ENTITY,
        REPAIR_ENTITY,
        ENVIRONMENT_BALANCER
    };
}

typedef entity_type::entity_type entity_t;

struct save
{
    entity_t type;
    byte_vector vec;
};

///entity->repair?
///seems a little too general
///but then again, all entities below can be repaired and destroyed, so
///maybe entity->can_repair?
struct entity
{
    vec2f position;
    float rotation;

    entity();

    virtual void tick(state& s, float dt){};
    ///so, virtual functions are basically function pointers
    ///which means that itll either call the base default, or
    ///the class overridden TICK function
    ///this is what to do while I am being held
    ///separate tick function because its gunna be real different for a whole bunch of things
    ///but at the same time lots of stuff will be the same
    //virtual void tick_held(state& s, float dt){ tick(s, dt); };

    ///I have just been used, and I am being carried by the entity specified below
    virtual void on_use(state&, float dt, entity*) {}

    virtual save make_save() = 0;

    virtual ~entity();

    ///remove me at the next available opportunity
    void schedule_unload();
    void schedule_delete();

    ///so that if a player picks me up, I've got a name
    virtual std::string get_display_info() {return "";};

    virtual void set_position(vec2f _pos);

    bool to_unload;
};

struct suit_entity;

///I'm going to add carrying functionality ot the player
///but later I'm going to set it up as a component
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
    momentum_handler momentum;

    std::vector<entity*> carried;

    void pickup(entity* en);
    entity* drop(int num);

    player();
    player(byte_fetch& fetch, state& s);

    float repair_suit(float amount);

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
    virtual void load(resource_network& net);

    virtual void set_position(vec2f _pos) override;

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

///absorbs co2, stores oxygen in tanks
struct oxygen_reclaimer : resource_entity
{
    renderable_circle circle;

    oxygen_reclaimer();
    oxygen_reclaimer(resource_network& _net);
    oxygen_reclaimer(byte_fetch& fetch);

    virtual void tick(state& s, float dt) override;

    save make_save();
};


///next we need an entity refilling station, as well as an environmental monitor component
///and then an entity that reads from that component and updates the world accordingly
///wait we already have an environment monitor
///really what we want is a conditional environmental monitor though
///nah, we can just use conditional_environment for it, even though thats
///not 100% what its built for
///what we need is somethign to sample the atmosphere, and then emit/absorb any incorrect deviations from standard
///including pressure!

struct environment_balancer : resource_entity
{
    resource_converter emitter;

    renderable_circle circle;
    conditional_environment_modifier environment;
    air_displayer air_quality;

    environment_balancer();
    environment_balancer(resource_network& _net);
    environment_balancer(byte_fetch& fetch);

    virtual void tick(state& s, float dt) override;

    void load(resource_network& net);

    save make_save();

    resource_network* net;
};


struct suit_entity : entity
{
    suit this_suit;
    renderable_file file;
    area_interacter interact;

    suit_entity();
    suit_entity(vec2f _pos);

    virtual void tick(state& s, float dt);

    virtual save make_save();

    std::string get_display_info() override;
};

///something that can be used to repair something else
///I'm an entity because I can be carried
///maybe we should have a repair component in here or something
///for repairing a suit
struct repair_entity : entity
{
    repair_component repair_amount;
    area_interacter interact;

    float add(float amount);
    float deplete(float amount);

    repair_entity();
    repair_entity(byte_fetch&);

    virtual void tick(state& s, float dt) override;
    virtual void on_use(state& s, float dt, entity* en);

    virtual save make_save();

    std::string get_display_info() override;
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
