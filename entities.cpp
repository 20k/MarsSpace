#include "entities.h"

#include "misc.h"

#include "sound.h"

entity::entity()
{
    position = (vec2f){0.f, 0.f};
    rotation = 0.f;
    to_unload = false;
    to_delete = false;
}

void entity::schedule_unload()
{
    to_unload = true;
}

void entity::schedule_delete()
{
    schedule_unload();

    to_delete = true;
}

entity::~entity()
{

}

void entity::set_position(vec2f _pos)
{
    position = _pos;
}

player::player()
{
    file.load("./res/character.png");

    ///should I define units right off the bat
    speed.set_speed(14.f);

    has_suit = true;

    momentum.set_mass(200.f);

    my_suit = new suit_entity(position);

    breath.lungs.set_parent(&my_suit->this_suit.environment);

    /*for(int i=0; i<air::RES_COUNT; i++)
    {
        carried_resources.set_max_storage({{(resource_t)i, 1.f}});
    }

    player_resource_network.add(&carried_resources);*/

    inventory_item_selected = 0;
}

float player::repair_suit(float amount)
{
    if(has_suit)
        return my_suit->this_suit.repair(amount);

    return amount;
}

void player::set_active_player(state& s)
{
    s.current_player = this;
}

///momentum is officially a huge clusterfuck, fixme
void player::tick(state& s, float dt)
{
    vec2f key_dir = key.tick();

    float cur_speed = speed.get_speed() * 1.5;

    if(has_suit)
        cur_speed = speed.get_speed();

    float slowdown_frac = 0.9999f;

    ///needs to be fixed to be frametime independent
    if(key_dir.length() < 0.001f)
    {
        slowdown_frac = 0.93f;
    }

    slowdown_frac = clamp(slowdown_frac, 0.f, 1.f);

    vec2f old_pos = position;

    position = momentum.do_movement(s, position, key_dir, cur_speed, dt, slowdown_frac);

    if((position - old_pos).length() != 0)
    {
        float next_rot = (position - old_pos).angle() + M_PI/2.f;

        ///oh god i hate interpolating round a circle

        float min_dist = circle_minimum_distance(rotation, next_rot);

        float weight = 3.f;

        rotation = rotation * weight + min_dist/2.f;
        rotation = rotation / weight;

        if(rotation > M_PI*2.f)
            rotation -= M_PI*2.f;
        if(rotation < 0)
            rotation += M_PI*2.f;
    }


    //file.tick(s, position, 0.1f); //old
    file.tick(s, position, 0.025f, rotation, true);

    auto air_parts = monitor.get_air_parts(s, position);

    display.tick(s, position + (vec2f){10.f, -10.f}, air_parts);

    breath.tick(s, position, dt);

    ///suit entity should have mass and be carried
    if(has_suit && my_suit)
    {
        my_suit->set_position(position);
        my_suit->tick_suit(s, dt);
        my_suit->this_suit.suit_display.tick(s, my_suit->this_suit);

        momentum.set_mass(100.f);
    }
    else
    {
        momentum.set_mass(10.f);
    }

    ///print currently selected element
    ///keyboard controls - arrow keys
    sf::Keyboard keyb;

    ///gradually getting less hitler
    if(keyb.isKeyPressed(sf::Keyboard::G))
    {
        if(carried.size() > 0)
        {
            carried[inventory_item_selected]->on_use(s, dt, this);
        }
    }

    std::string display_total = "   Inventory:\n\n";

    for(int i=0; i<carried.size(); i++)
    {
        std::string extra = "   ";

        if(i == inventory_item_selected)
            extra = "-->";

        display_total = display_total + extra + carried[i]->get_display_info() + "\n";
    }

    if(carried.size() == 0)
        display_total = display_total + "   None\n";

    text txt;
    txt.render(s, display_total, (vec2f){s.win->getSize().x - 200.f, 40.f}, 10, text_options::ABS);

    if(breath.lungs.get_parent_pressure(s, position) < music::low_air_threshold)
    {
        music::swap_to_song_type(music::LOWAIR);
    }

    carried_display.tick(s, (vec2f){700.f, 20.f}, player_resource_network.network_resources, 10, true);

    ///still unsure about lumpy distribution. Ideally we'd want to take from the currently
    ///selected resource or the nearest... maybe? Or perhaps lumpy *is* best?
    player_resource_network.tick(s, dt, true);
}

///we need to set_active the player when loading
///want to save carried resources eventually
///wait, maybe we can literally do this through the save component
///I might be a genius
///I'll have to chop it up a bit, but that is hilarious
player::player(byte_fetch& fetch, state& s) : player()
{
    ///temp hack. Should the player just own this?
    ///or should I incorporate it into the save system
    file.load("res/character.png");

    position = fetch.get<vec2f>();
    rotation = fetch.get<float>();
    float sp = fetch.get<float>();

    vecrf breather_environment = fetch.get<vecrf>();

    breath.lungs.my_environment.local_environment = breather_environment;

    has_suit = fetch.get<int32_t>();

    //player_resource_network.add(fetch.get<vecrf>());

    ///temporarily broken as there is no inventory to save :[
    inventory_item_selected = fetch.get<int32_t>();

    ///terminate player fetching
    if(has_suit)
        my_suit->load(fetch);

    if(!has_suit)
        file.load("./res/nosuit.png");

    int carry_num = fetch.get<int32_t>();

    for(int i=0; i<carry_num; i++)
    {
        saver sav;
        entity* en = sav.fetch_next_entity(fetch, s);

        pickup(en);
    }

    speed.set_speed(sp);

    ///????
    set_active_player(s);
}

save player::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<float>(rotation);
    vec.push_back<float>(speed.get_speed());

    ///??? figure out a better way to do this
    ///Or is this ok?
    vec.push_back<vecrf>(breath.lungs.my_environment.local_environment);
    vec.push_back<int32_t>(has_suit);

    vec.push_back<int32_t>(inventory_item_selected);

    if(has_suit)
        vec.push_back(my_suit->make_save().vec);

    vec.push_back<int32_t>(carried.size());

    for(int i=0; i<(int)carried.size(); i++)
    {
        save ms = carried[i]->make_save();

        vec.push_back<entity_t>(ms.type);
        vec.push_back(ms.vec);
    }

    return {entity_type::PLAYER, vec};
}

///so, for the moment the suit only functions as an atmospheric environment
///no need to create a proper suit update
///however, we're going to want to store a pointer to a suit entity later
void player::set_suit(suit_entity* en)
{
    breath.lungs.set_parent(&en->this_suit.environment);
    my_suit = en;
    has_suit = true;
    file.load("./res/character.png");
    rotation = en->rotation;
}

void player::remove_suit()
{
    breath.lungs.remove_parent();
    has_suit = false;
    file.load("./res/nosuit.png");
}

suit_entity* player::drop_suit()
{
    if(!has_suit)
        return nullptr;

    remove_suit();

    my_suit->set_position(position);
    my_suit->rotation = rotation;

    return my_suit;
}

void player::pickup(entity* en)
{
    resource_packet* pack = dynamic_cast<resource_packet*>(en);

    if(pack != nullptr)
    {
        player_resource_network.add(&pack->conv);
    }

    carried.push_back(en);
}

entity* player::drop(int num)
{
    if(num < 0 || num >= (int)carried.size())
        return nullptr;

    entity* en = carried[num];
    en->set_position(position);

    resource_packet* pack = dynamic_cast<resource_packet*>(en);

    if(pack != nullptr)
    {
        player_resource_network.rem(&pack->conv);
    }

    carried.erase(carried.begin() + num);

    return en;
}

entity* player::drop_current()
{
    return drop(inventory_item_selected);
}

void player::inc_inventory()
{
    inventory_item_selected += 1;
    inventory_item_selected = clamp(inventory_item_selected, 0, (int)carried.size()-1);

    if(carried.size() == 0)
        inventory_item_selected = 0;
}

void player::dec_inventory()
{
    inventory_item_selected -= 1;
    inventory_item_selected = clamp(inventory_item_selected, 0, (int)carried.size()-1);

    if(carried.size() == 0)
        inventory_item_selected = 0;
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
        i.tick(s, dt);
}

building::building(byte_fetch& fetch, state& s)
{
    int32_t num = fetch.get<int32_t>();

    for(int i=0; i<num; i++)
    {
        auto start = fetch.get<vec2f>();
        auto finish = fetch.get<vec2f>();

        add_wall(s, start, finish);

        int sub_num = fetch.get<int32_t>();

        if(sub_num == 0)
            walls.back().sub_segments.clear();

        for(auto& j : walls.back().sub_segments)
        {
            j.construct.achieved_work = fetch.get<float>();
            j.res_require.res_added = fetch.get<vecrf>();
        }
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

        vec.push_back<int32_t>(i.sub_segments.size());

        for(auto& j : i.sub_segments)
        {
            vec.push_back<float>(j.construct.achieved_work);
            vec.push_back<vecrf>(j.res_require.res_added);
        }
    }

    //vec.push_back<int32_t>()

    return {entity_type::BUILDING, vec};
}

door::door(vec2f _start, vec2f _finish, float time_to_open) :
    open(time_to_open),
    i1((_finish - _start).rot(M_PI/2.f).norm() * 3.f + (_start + _finish)/2.f, 2.f), ///temp
    i2((_finish - _start).rot(-M_PI/2.f).norm() * 3.f + (_start + _finish)/2.f, 2.f), ///temp
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
    display.tick(s, position + (vec2f){10.f, -10.f}, conv.local_storage, 16, false);
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

resource_packet::resource_packet(resource_t _type)
{
    load(_type);
}

void resource_packet::load(resource_t _type)
{
    type = _type;

    ///number out of a hat for the moment
    conv.set_max_storage({{type, 10.f}});
    conv.local_storage.v[type] = 10.f;

    display.set_element_to_display(type);
}

resource_packet::resource_packet(byte_fetch& fetch)
{
    type = fetch.get<resource_t>();

    resource_packet::load(type);

    position = fetch.get<vec2f>();
    conv.local_storage = fetch.get<vecrf>();

    //printf("%f\n", conv.local_storage.v[type]);
}

void resource_packet::tick(state& s, float dt)
{
    interact.set_position(position);
    interact.set_radius(2.f);
    interact.tick(s);

    txt.render(s, air::short_names[type], position, 16, text_options::CENTERED);

    if(interact.player_has_interacted(s))
    {
        schedule_unload();

        s.current_player->pickup(this);
    }
}

void resource_packet::on_use(state& s, float dt, entity* parent)
{
    player* play = dynamic_cast<player*>(parent);

    if(play == nullptr)
        return;

    //printf("pre %f\n", conv.local_storage.v[type]);

    /**vecrf extra = play->player_resource_network.add(conv.local_storage);
    conv.local_storage = extra;**/

    //printf("post %f\n", extra.v[type]);
}

save resource_packet::make_save()
{
    byte_vector vec;
    vec.push_back<resource_t>(type);
    vec.push_back<vec2f>(position);
    vec.push_back<vecrf>(conv.local_storage);

    //printf("%f\n", conv.local_storage.v[type]);

    return {entity_type::RESOURCE_PACKET, vec};
}

std::string resource_packet::get_display_info()
{
    return air::names[type] + ": " + std::to_string(conv.local_storage.v[type]);
}

solar_panel::solar_panel()
{
    conv.set_max_storage({{resource::POWER, 0.1f}});
    conv.set_output_ratio({{resource::POWER, 1.f}});
    conv.set_amount(900); ///watts
    file.load("./res/solar_panel.png");

    display.set_element_to_display(resource::POWER);
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

    display.set_element_to_display(resource::POWER);
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
    float liquid_litres = 50.f;

    ///eh. Precise accuracy, I don't care atm
    ///we can change this to an accurate map later
    float gas_litres = liquid_litres * air::liquid_to_gas_conversion_ratio_c02;

    type = _type;
    conv.set_max_storage({{type, gas_litres}}); ///litres
    display.set_element_to_display(type);
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

    float gas_accounted_litres_ps = air::liquid_to_gas_conversion_ratio_c02 * litres_ps;

    float game_speed = 1000.f;

    ///all resources are per second. So 1 watt produces litresps c02
    ///well, it should be litres_ps, but unfortunately we cant use realistic values
    ///otherwise itll take 1.5 actual years to play the game
    ///and as exciting as that is, its probably not ideal to build a playerbase
    //conv.set_absorption_rate(gas_accounted_litres_ps * game_speed);
    conv.set_air_transfer_rate(gas_accounted_litres_ps * game_speed);
    conv.set_max_storage({{resource::C02, 1.f}});
    conv.set_usage_ratio({{resource::POWER, 1000.f}});
    conv.set_usage_ratio({{resource::C02, gas_accounted_litres_ps}});
    conv.set_output_ratio({{resource::OXYGEN, 1.f}});
    conv.set_efficiency(gas_accounted_litres_ps / (1000.f + gas_accounted_litres_ps));
    conv.set_amount(1000.f + gas_accounted_litres_ps); ///per s

    ///none
    display.set_element_to_display((resource_t)resource::C02);
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

environment_balancer::environment_balancer()
{
    conv.set_max_storage({{resource::C02, 0.1f}});

    ///bad
    ///really bad
    net = nullptr;

    display.set_element_to_display((resource_t)resource::RES_COUNT);
}

environment_balancer::environment_balancer(resource_network& _net) : environment_balancer()
{
    ///object fully initialised, not illegal to do this
    ///if we ever call load from in the resource entity constructor though
    ///that is very illegals
    load(_net);
}

environment_balancer::environment_balancer(byte_fetch& fetch) : environment_balancer()
{
    resource_entity::load(fetch);
}

void environment_balancer::load(resource_network& _net)
{
    net = &_net;

    ///we need to set conv.parent and emitter.parent to be the correct environment
    ///which means they need to actually work as well
}

save environment_balancer::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<vecrf>(conv.local_storage);

    return {entity_type::ENVIRONMENT_BALANCER, vec};
}

void environment_balancer::set_parent(conditional_environment_modifier* parent)
{
    environment.set_parent(parent);
}

void environment_balancer::process_environment(state& s, float dt)
{
    static vecrf ideal_environment = air_to_resource(get_controlled_environment_atmosphere());

    vecrf parent_air = environment.get_parent(s, position);

    vecrf to_ideal = ideal_environment - parent_air;

    ///everything > 0
    vecrf to_emit = max(to_ideal, 0.f);
    ///everything < 0
    vecrf to_absorb = max(-to_ideal, 0.f);

    ///again what we really want is an atomic swap
    ///also, it turns out I was really wrong in normalising the conversion parameters
    ///Im going to have to change the api because it sucks

    float air_processed_per_second = 1.f;

    environment.set_max_air(air_processed_per_second);
    environment.absorb_all(s, position, air_processed_per_second * dt);

    auto removed = environment.take(to_absorb);

    auto can_emit = net->take(to_emit);

    auto extra = environment.add(can_emit);

    environment.emit_all(s, position, air_processed_per_second * dt * 10.f);

    /*printf("Rem %f\n", removed.v[air::OXYGEN]);
    printf("Add %f\n", extra.v[air::OXYGEN]);
    printf("store %f\n", net->network_resources.v[air::OXYGEN]);*/

    ///for the moment destroying resources
    net->add(removed);
    net->add(extra);
}

///so, this tries to maintain a certain volume/pressure of gas essentially
///but doesn't care about its ratios to other gases
///I have no idea how scientifically accurate this is
void environment_balancer::tick(state& s, float dt)
{
    process_environment(s,dt);

    circle.tick(s, position, 1.f, (vec4f({255, 100, 255, 255})));
}

suit_entity::suit_entity()
{
    interact.set_radius(2.f);
    file.load("./res/character.png");
}

suit_entity::suit_entity(vec2f _pos) : suit_entity()
{
    set_position(_pos);
}

void suit_entity::tick_suit(state& s, float dt)
{
    this_suit.tick(s, dt, position);
}

///area interactor position needs to be where the player was standing when they dropped the suit
///but enough of that for the moment
void suit_entity::tick(state& s, float dt)
{
    interact.set_position(position);

    ///destroy me, add to player
    if(interact.player_has_interacted(s))
    {
        s.current_player->set_suit(this);
        schedule_unload();
    }

    interact.tick(s);
    file.tick(s, position, 0.025f, rotation, true);

    tick_suit(s, dt);
}

suit_entity::suit_entity(byte_fetch& fetch) : suit_entity()
{
    load(fetch);
}

void suit_entity::load(byte_fetch& fetch)
{
    position = fetch.get<vec2f>();
    this_suit.environment.my_environment.local_environment = fetch.get<vecrf>();

    auto saved_resources = fetch.get<vecrf>();

    rotation = fetch.get<float>();

    for(auto& i : this_suit.parts)
    {
        i.second.damage.health = fetch.get<float>();
    }

    this_suit.suit_resource_network.network_resources = saved_resources;
    this_suit.resource_storage.local_storage = 0.f;
}

save suit_entity::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<vecrf>(this_suit.environment.my_environment.local_environment);
    vec.push_back<vecrf>(this_suit.suit_resource_network.network_resources);
    vec.push_back<float>(rotation);

    for(auto& i : this_suit.parts)
    {
        vec.push_back<float>(i.second.damage.health);
    }

    return {entity_type::SUIT_ENTITY, vec};
}


std::string suit_entity::get_display_info()
{
    return "Suit";
}

repair_entity::repair_entity()
{
    interact.set_radius(1.5f);
}

repair_entity::repair_entity(byte_fetch& fetch) : repair_entity()
{
    position = fetch.get<vec2f>();
    repair_amount.repair_remaining = fetch.get<float>();
}

float repair_entity::add(float amount)
{
    return repair_amount.add(amount);
}

float repair_entity::deplete(float amount)
{
    return repair_amount.deplete(amount);
}

save repair_entity::make_save()
{
    byte_vector vec;
    vec.push_back<vec2f>(position);
    vec.push_back<float>(repair_amount.repair_remaining);

    return {entity_type::REPAIR_ENTITY, vec};
}

void repair_entity::tick(state& s, float dt)
{
    if(to_unload)
        return;

    interact.set_position(position);

    if(interact.player_has_interacted(s))
    {
        s.current_player->pickup(this);
        schedule_unload();
    }

    interact.tick(s, false);

    text txt;
    txt.render(s, get_display_info(), position);
}

void repair_entity::on_use(state& s, float dt, entity* en)
{
    player* play = dynamic_cast<player*>(en);

    if(play == nullptr)
        return;

    float repair_speed = 0.1f;

    float amount = deplete(repair_speed * dt);

    printf("Remaining %f\n", repair_amount.repair_remaining);

    float extra = play->repair_suit(amount);

    add(extra);
}

std::string repair_entity::get_display_info()
{
    return std::string("Repair Material: ") + std::to_string(repair_amount.repair_remaining);
}

///dunnae do anything!
/*void repair_entity::tick_held(state& s, float dt)
{

}*/
