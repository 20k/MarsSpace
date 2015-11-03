#include "entities.h"

entity::entity()
{
    position = (vec2f){0.f, 0.f};
    rotation = 0.f;
    to_unload = false;
}

void entity::schedule_unload()
{
    to_unload = true;
}

entity::~entity()
{

}

player::player()
{
    file.load("./res/character.png");

    ///should I define units right off the bat
    speed.set_speed(14.f);

    breath.lungs.set_parent(&mysuit.environment);

    has_suit = true;

    momentum.set_mass(200.f);
}

void player::set_active_player(state& s)
{
    s.current_player = this;
}

void player::tick(state& s, float dt)
{
    vec2f key_dir = key.tick(1.f).norm();

    ///this is a badly named function, it simply stores my speed modifier
    //float cur_speed = speed.get_speed() * dt * 2; ///temporary hack until i get my shit together

    float cur_speed = speed.get_speed() * 2 * dt;

    if(has_suit)
        cur_speed = cur_speed / 2.f;

    //position = mover.tick(s, position, key_dir, cur_speed);
    ///cur_speed currently used as max speed, which is.... not what i want at all
    ///also this function isn't time accurate

    float slowdown_frac = 0.9999f * (dt / 0.01f);

    if(key_dir.length() == 0)
    {
        slowdown_frac = 0.91f * (dt / 0.01f);
    }

    slowdown_frac = clamp(slowdown_frac, 0.f, 1.f);

    vec2f old_pos = position;

    position = momentum.do_movement(s, position, key_dir, cur_speed, dt, slowdown_frac);

    if((position - old_pos).length() != 0)
    {
        float next_rot = (position - old_pos).angle() + M_PI/2.f;

        ///oh god i hate interpolating round a circle

        float min_dist = circle_minimum_distance(rotation, next_rot);

        //printf("%f %f %f\n", rotation, next_rot, min_dist);

        float weight = 3.f;

        rotation = rotation * weight + min_dist/2.f;
        rotation = rotation / weight;

        if(rotation > M_PI*2.f)
            rotation -= M_PI*2.f;
        if(rotation < 0)
            rotation += M_PI*2.f;
    }


    //file.tick(s, position, 0.1f); //old
    file.tick(s, position, 0.025f, rotation);

    auto air_parts = monitor.get_air_parts(s, position);

    display.tick(s, position + (vec2f){10.f, -10.f}, air_parts);

    breath.tick(s, position, dt);

    if(has_suit)
    {
        mysuit.tick(s, dt, position);
        momentum.set_mass(100.f);
    }
    else
    {
        momentum.set_mass(10.f);
    }

}

///we need to set_active the player when loading
player::player(byte_fetch& fetch, state& s) : player()
{
    ///temp hack. Should the player just own this?
    ///or should I incorporate it into the save system
    file.load("res/character.png");

    position = fetch.get<vec2f>();
    float sp = fetch.get<float>();

    vecrf breather_environment = fetch.get<vecrf>();
    vecrf suit_environment = fetch.get<vecrf>();

    breath.lungs.my_environment.local_environment = breather_environment;
    mysuit.environment.my_environment.local_environment = suit_environment;

    has_suit = fetch.get<int32_t>();

    speed.set_speed(sp);

    ///????
    set_active_player(s);
}

save player::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<float>(speed.get_speed());

    ///??? figure out a better way to do this
    ///Or is this ok?
    vec.push_back<vecrf>(breath.lungs.my_environment.local_environment);
    vec.push_back<vecrf>(mysuit.environment.my_environment.local_environment);
    vec.push_back<int32_t>(has_suit);

    return {entity_type::PLAYER, vec};
}

///so, for the moment the suit only functions as an atmospheric environment
///no need to create a proper suit update
///however, we're going to want to store a pointer to a suit entity later
void player::set_suit(suit& s)
{
    breath.lungs.set_parent(&mysuit.environment);
    has_suit = true;
}

void player::remove_suit()
{
    breath.lungs.remove_parent();
    has_suit = false;
}

suit_entity* player::drop_suit()
{
    if(!has_suit)
        return nullptr;

    suit_entity* s = new suit_entity(position);

    remove_suit();

    return s;
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

///I think the resource network may need to become an entity
resource_entity::resource_entity(resource_network& net)
{
    net.add(&conv);

    position = (vec2f){0, 0};
}

void resource_entity::set_position(vec2f pos)
{
    position = pos;

    conv.set_position(position);
}

void resource_entity::tick(state& s, float dt)
{
    display.tick(s, position + (vec2f){15.f, -10.f}, conv.local_storage);
}

save resource_entity::make_save()
{
    return {entity_type::RESOURCE_ENTITY, byte_vector()};
}

resource_entity::resource_entity(byte_fetch& fetch)
{

}

void resource_entity::load(byte_fetch& fetch)
{
    position = fetch.get<vec2f>();
    conv.local_storage = fetch.get<vecrf>();
}

void resource_entity::load(resource_network& net)
{
    net.add(&conv);
}

resource_entity::resource_entity()
{

}

solar_panel::solar_panel()
{
    conv.set_max_storage({{resource::POWER, 0.1f}});
    conv.set_output_ratio({{resource::POWER, 1.f}});
    conv.set_amount(900); ///watts
    file.load("./res/solar_panel.png");

}

solar_panel::solar_panel(resource_network& net) : solar_panel()
{
    load(net);
}

solar_panel::solar_panel(byte_fetch& fetch) : solar_panel()
{
    load(fetch);
}

void solar_panel::tick(state& s, float dt)
{
    resource_entity::tick(s, dt);

    file.tick(s, position, 0.2f);
}

///we're gunna need to save resource network ids
///and then recreate them :[
save solar_panel::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<vecrf>(conv.local_storage);

    return {entity_type::SOLAR_PANEL, vec};
}

hydrogen_battery::hydrogen_battery()
{
    conv.set_max_storage({{resource::POWER, 9 * 1000 * 1000.f}});
}

hydrogen_battery::hydrogen_battery(resource_network& net) : hydrogen_battery()
{
    load(net);
}

hydrogen_battery::hydrogen_battery(byte_fetch& fetch) : hydrogen_battery()
{
    load(fetch);
}


void hydrogen_battery::tick(state& s, float dt)
{
    resource_entity::tick(s, dt);
    ///orange
    circle.tick(s, position, 5.f, (vec4f){255, 140, 0, 255});

    txt.render(s, air::short_names[air::POWER], position, 24, (text_options::text_options)(text_options::CENTERED | text_options::OUTLINED));
}

save hydrogen_battery::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<vecrf>(conv.local_storage);

    return {entity_type::HYDROGEN_BATTERY, vec};
}

gas_storage::gas_storage(air_t _type)
{
    type = _type;
    conv.set_max_storage({{type, 50.f}}); ///litres
}

gas_storage::gas_storage(resource_network& net, air_t _type) : gas_storage(_type)
{
    load(net);
}


gas_storage::gas_storage(byte_fetch& fetch) : gas_storage(fetch.get<air_t>())
{
    load(fetch);
}

void gas_storage::tick(state& s, float dt)
{
    resource_entity::tick(s, dt);

    float rad = 5.f;

    circle.tick(s, position, rad, (vec4f){100, 255, 255, 255});

    txt.render(s, air::short_names[type], position, 24, (text_options::text_options)(text_options::CENTERED | text_options::OUTLINED));
}

save gas_storage::make_save()
{
    byte_vector vec;
    vec.push_back<air_t>(type);
    vec.push_back<vec2f>(position);
    vec.push_back<vecrf>(conv.local_storage);

    return {entity_type::GAS_STORAGE, vec};
}

oxygen_reclaimer::oxygen_reclaimer()
{

    float litres_per_hour = 0.5f;
    float litres_per_minute = litres_per_hour / 60.f;
    float litres_ps = litres_per_minute / 60.f;
    float liquid_to_gas_conversion_ratio_oxygen = 861;
    float liquid_to_gas_conversion_ratio_c02 = 845;

    float gas_accounted_litres_ps = liquid_to_gas_conversion_ratio_c02 * litres_ps;

    float game_speed = 1000.f;

    ///all resources are per second. So 1 watt produces litresps c02
    ///well, it should be litres_ps, but unfortunately we cant use realistic values
    ///otherwise itll take 1.5 actual years to play the game
    ///and as exciting as that is, its probably not ideal to build a playerbase
    conv.set_absorption_rate(gas_accounted_litres_ps * game_speed);
    conv.set_max_storage({{resource::C02, 1.f}});
    conv.set_usage_ratio({{resource::POWER, 1000.f}});
    conv.set_usage_ratio({{resource::C02, gas_accounted_litres_ps}});
    conv.set_output_ratio({{resource::OXYGEN, 1.f}});
    conv.set_efficiency(gas_accounted_litres_ps / (1000.f + gas_accounted_litres_ps));
    conv.set_amount(1000.f + gas_accounted_litres_ps); ///per s
}

oxygen_reclaimer::oxygen_reclaimer(resource_network& _net) : oxygen_reclaimer()
{
    load(_net);
}


oxygen_reclaimer::oxygen_reclaimer(byte_fetch& fetch) : oxygen_reclaimer()
{
    load(fetch);
}


void oxygen_reclaimer::tick(state& s, float dt)
{
    resource_entity::tick(s, dt);

    circle.tick(s, position, 1.f, (vec4f({100, 100, 255, 255})));
}

save oxygen_reclaimer::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<vecrf>(conv.local_storage);

    return {entity_type::OXYGEN_RECLAIMER, vec};
}

suit_entity::suit_entity()
{
    interact.set_radius(2.f);
    file.load("./res/suit.png");
}

suit_entity::suit_entity(vec2f _pos) : suit_entity()
{
    set_position(_pos);
}

///area interactor position needs to be where the player was standing when they dropped the suit
///but enough of that for the moment
void suit_entity::tick(state& s, float dt)
{
    interact.set_position(position);

    ///destroy me, add to player
    if(interact.player_has_interacted(s))
    {
        s.current_player->set_suit(this_suit);
        schedule_unload();
    }

    interact.tick(s);
    file.tick(s, position, 0.1f);
}

save suit_entity::make_save()
{
    return {entity_type::SUIT_ENTITY, byte_vector()};
}

void suit_entity::set_position(vec2f _pos)
{
    position = _pos;
}
