#ifndef COMPONENTS_H_INCLUDED
#define COMPONENTS_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>
#include <vector>
#include <set>
#include <memory>
#include "air.hpp"

///this project is an experiment in completely modularising all functionality
///into very tiny components, then embedding them into entities
///to see how that plays out
///this is very divergent from how i normally code

struct entity;
struct movement_blocker;
struct resource_network;
struct player;
struct environment_balancer;

struct state
{
    sf::RenderWindow* win;
    ///also need movement blockers in here
    std::vector<std::shared_ptr<movement_blocker>> blockers;

    ///or maybe I should make this a std vector for genericness?
    ///use for activating stuff
    player* current_player;

    sf::Texture planet_tex;

    air_processor* air_process = nullptr;

    ///from the centre of the map
    vec2f sun_direction;

    ///nullptr not an acceptable state
    std::vector<entity*>* entities = nullptr;

    float* iron_map = nullptr;

    vec2f dimensions;

    state(sf::RenderWindow* _win, sf::Texture&, air_processor&);
};

struct renderable_file
{
    sf::Image img;
    sf::Texture tex;
    sf::RenderTexture* rtex;

    void load(const std::string&Y);
    void tick(state& s, vec2f pos, float scale, float rotation = 0.f, bool shadow = false, bool absolute = false, sf::RenderStates rs = sf::RenderStates());
};

struct renderable_texture
{
    sf::Texture tex;

    void load(sf::Texture& _tex);
    void tick(state& s, vec2f pos);
};

struct renderable_circle
{
    void tick(state& s, vec2f pos, float rad, vec4f col, float outline_thickness = 0.5f, float outline_factor = 0.5f);
};

struct renderable_rectangle
{
    void tick(state& s, vec2f start, vec2f finish, float thickness, vec4f col = (vec4f){190, 190, 190, 255}, float outline_thickness = 0.5f);
};

struct constructable
{
    float max_work;
    float achieved_work;

    void set_work_to_complete(float amount);
    void apply_work(float amount);
    bool is_constructed();
    float get_completed_frac();

    constructable();
};

/*struct footprint_leaver
{
    std::vector<renderable_file> footprints;


};*/

///implicitly coupled with constructable
struct resource_requirer
{
    vecrf res_required;
    vecrf res_added;

    resource_requirer();

    void set_resource_requried(resource_t type, float max_resource);

    vecrf add(const vecrf& res);

    float get_resource_amount_required_to_complete_fraction(float frac);

    float get_completed_frac();
    bool is_completed();
};

///this may look like the worst class in the entire universe, but i guarantee you itll be more useful later
///gets blocked by stuff
struct moveable
{
    vec2f tick(state& s, vec2f position, vec2f dir, float dist);
};

struct keyboard_controller
{
    vec2f tick();
};

struct speed_handler
{
    float speed;

    void set_speed(float _speed);
    float get_speed();
};

struct movement_blocker
{
    vec2f start;
    vec2f finish;

    movement_blocker(vec2f _start, vec2f _finish);

    void push_remote(state& s);
    void destroy_remote(state& s);

    void tick(state& s);
    void modify_bounds(vec2f _start, vec2f _finish);

    uint32_t id;
    static uint32_t gid;

    std::shared_ptr<movement_blocker> remote;
};

///we can give this a watch, ie an entity that it is looking at
///makes it hard to save unless we go down the unique id route for all entities in a map
///which is not not doable, but is quite a large annoyance factor
///although we can do that super automatically through the entity class
struct area_interacter
{
    renderable_circle circle;
    vec2f pos;
    float radius;

    ///maybe remove _pos and radius from the constructor, as we might want to
    ///interact with moving entities (eg get in a sliding rover)
    area_interacter(vec2f _pos, float _radius);
    area_interacter();

    void set_position(vec2f _pos);
    void set_radius(float rad);

    void tick(state& s, bool gradient_centre = true);

    std::vector<entity*> get_entities_within(state& s);
    bool player_inside(state& s);
    bool player_has_interacted(state& s); ///will only fire once per button hold
    bool player_has_interacted_continuous(state& s); ///will fire continuously

private:
    bool just_interacted;
};

struct wall_segment_segment
{
    vec2f start, finish;
    constructable construct;
    resource_requirer res_require;

    renderable_rectangle rect;

    area_interacter i1, i2;

    wall_segment_segment(vec2f _start, vec2f _finish, float required_work);

    void tick(state& s, float dt);
};

struct wall_segment
{
    movement_blocker block;
    renderable_rectangle rect;

    std::vector<wall_segment_segment> sub_segments;

    vec2f start, finish;

    wall_segment(vec2f _start, vec2f _finish, float work_per_segment);

    wall_segment split_at_fraction(state& s, float frac);

    void generate_sub_segments(float work_per_segment);
    void tick(state& s, float dt);
    void destroy(state& s);

    float work;
};

struct wall_splitter
{
    std::vector<wall_segment> split(state& s, float frac, wall_segment& seg);
};

struct mouse_fetcher
{
    vec2f get_world(state& s);
    vec2f get_screen(state& s);
};

///literally just goes from 0 -> 1, or 1 -> 0 with a time delay
struct opener
{
    float time_duration;

    ///time in ms
    opener(float _time);

    void open();
    void close();
    void toggle();

    float get_open_fraction();
    void tick(float dt);

private:
    float open_frac;
    float direction; ///1 for opening, -1 for closing, 0 for stable
};

struct squasher
{
    vec2f get_squashed_end(vec2f start, vec2f finish, float squash_fraction);
};

struct byte_fetch;

struct saver
{
    void save_to_file(const std::string& fname, const std::vector<entity*> stuff);
    std::vector<entity*> load_from_file(const std::string& fname, state& s);
    entity* fetch_next_entity(byte_fetch&, state& s);
};

namespace text_options
{
    enum text_options : uint32_t
    {
        NONE = 0,
        CENTERED = 1,
        ABS = 2,
        OUTLINED = 4
    };
}

struct text
{
    std::string str;
    vec2f tl;

    ///need to load then render really
    void render(state& s, const std::string& _str, vec2f _tl, int size = 16, text_options::text_options opt = text_options::NONE);
};

///should probably rename this to air_environment_monitor or something
///it is NOT for local environments
struct air_monitor
{
    vec<air::COUNT, float> get_air_fractions(state& s, vec2f pos);
    vec<air::COUNT, float> get_air_parts(state& s, vec2f pos);

    float get_air_pressure(state& s, vec2f pos);
};

struct air_displayer
{
    text txt;

    void tick(state& s, vec2f display_pos, const vec<air::COUNT, float>& air_parts, bool absolute = false);
};

struct resource_displayer
{
    std::map<resource_t, bool> should_display;

    void set_element_to_display(resource_t type, bool val = true);

    text txt;

    void tick(state& s, vec2f display_pos, const vecrf& resources, int size, bool absolute);
};

struct environmental_gas_emitter
{
    void emit(state& s, vec2f pos, float amount, air_t type);
};

///we want to change this later so that the absorption rate is dependent on the amount left
///that way low o2 will kill us, not no o2
///although... its also pressure dependent
///so.... pv=nrt it? need to work out pv=nrt
///for normal human breathing, then do fractional bits to work out
///pressure, and v is absolute amount
///then ignore t
struct environmental_gas_absorber
{
    float absorb(state& s, vec2f pos, float amount, air_t type);
};

struct resource_converter;

///we want a maximum of all gas?
struct air_environment
{
    environmental_gas_absorber absorber;
    environmental_gas_emitter emitter;

    vecrf local_environment;

    ///this is specifically diffusion (active or passive)
    ///into the local environment
    ///ie breathing to a player
    ///or air -> rover
    void absorb_all(state& s, vec2f pos, float amount, float max_total);
    void emit_all(state& s, vec2f pos, float amount);

    ///assumes a perfect conversion
    ///can obvs make this lossy later
    void convert_percentage(float amount, float fraction, air_t input, air_t output);

    ///change of heart, doing saving in here means
    ///that to avoid having to leak implementation details
    ///i'd also need to do loading in here
    ///which is impractibly a pain in the arse
    //byte_vector get_save_data();

    //bool convert_amount(float amount, float fraction, air_t input, air_t output);

    //void absorb(state& s, vec2f pos, float amount, float maximum, air_t type);
    //void emit(state& s, vec2f pos, float amount, air_t type);

    air_environment();
};

///either we draw from the environment and emit to it if we can
///or we draw from and emit to the parent
///this is just functionality to allow this to happen
///it does not mandate that it must
///we need a volume for air_environment
///so that we can functionally get pressure
struct conditional_environment_modifier
{
    air_environment my_environment;

    conditional_environment_modifier* parent;

    void absorb_all(state& s, vec2f pos, float amount);
    void emit_all(state& s, vec2f pos, float amount);
    void set_max_air(float _max);

    void set_parent(conditional_environment_modifier* parent);
    void remove_parent();

    float get_pressure();
    float get_parent_pressure(state& s, vec2f pos);

    conditional_environment_modifier();

    vecrf take(float amount);
    vecrf take(vecrf amount);
    vecrf add(vecrf amount);

    vecrf get();
    vecrf get_parent(state& s, vec2f pos);

    float max_air = 0.f;
};

///define lethal levels, integrate with breather or at least provide easy api integration
///would quite like to model nitrogen too
struct body_model
{
    float pa_o2;
    float pa_co2;

    body_model();

    float get_gas_blood_volume_amount_atmospheric_ps_litres(float higher_pa, float lower_pa, float outside_pressure);
    float get_o2_blood_volume_used_atmospheric_ps_litres(float outside_atmospheric_pressure);
    float get_co2_blood_volume_used_atmospheric_ps_litres(float outside_atmospheric_pressure);

    void tick(float dt, float lung_pa_o2, float lung_pa_co2, float lung_volume, float lung_pa);
};

struct breather
{
    //air_environment lungs;
    conditional_environment_modifier lungs;
    air_displayer display;
    air_monitor monitor;
    body_model body;

    breather();

    void set_holding_breath_enabled(bool state);
    void tick(state& s, vec2f pos, float dt);
    ///set/remove parent?

    float current_time;

    bool is_holding_breath;
};

///for one resource or?
struct resource_converter
{
    air_environment environment_absorption;

    vecrf local_storage;
    vecrf max_storage;

    ///eg oxygen:1, c02:1
    vecrf conversion_usage_ratio;
    ///eg oxygen:2
    vecrf conversion_output_ratio;

    float amount;
    float efficiency;
    float environment_absorb_rate;
    float environment_emit_rate;

    vec2f pos;

    ///the above two examples combined would convert 1 o2 + 1c02 -> 202 (rubbish but)

    void set_max_storage_vec(const vecrf& vec);
    void set_max_storage(const std::vector<std::pair<resource_t, float>>& vec);
    void set_usage_ratio(const std::vector<std::pair<resource_t, float>>& vec);
    void set_output_ratio(const std::vector<std::pair<resource_t, float>>& vec);

    void set_usage(const vecrf& in);
    void set_output(const vecrf& out);

    void set_air_absorb_rate(float _rate);
    void set_air_emit_rate(float _rate);
    void set_air_transfer_rate(float _rate);

    ///maybe constructor me
    void set_amount(float amount);
    void set_efficiency(float efficiency);
    void set_position(vec2f _pos);

    void add(const std::vector<std::pair<resource_t, float>>& vec);
    vecrf take(const std::vector<std::pair<resource_t, float>>& vec);

    vecrf add(const vecrf& amount); ///returns any left over
    vecrf take(const vecrf& amount); ///returns amount actually taken

    void absorb_all(state& s, float dt);
    void emit_all(state& s, float dt);

    void convert(vecrf& global_storage, vecrf& global_max, float dt);

    resource_converter();
};

///maybe allow resource networks to connect to other resource networks, and then can recurse
///this is a redistributing resource network
///ie the network resources are proportionally shuttled between the elements
///we also need a non redistributing resource network :[
///these finally have to work together
struct resource_network
{
    vecrf network_resources;
    vecrf max_network_resources;

    std::vector<resource_converter*> converters;
    std::vector<resource_network*> connected_networks;

    ///add unique only adds something if its not already present
    void add_net(resource_network* net);

    void add_unique(resource_converter* conv);
    void add(resource_converter* conv);
    void rem(resource_converter* rem);
    void clear();

    ///returns what we couldn't add
    vecrf add(const vecrf& res);
    ///returns what we could take
    vecrf take(const vecrf& res);

    ///default is proportional allocation
    ///lump means that the maximum is placed in the first, then the next etc
    void tick(state& s, float dt, bool lump = false);

    resource_network();

    vecrf get_local_max();
    vecrf get_local_available();

    vecrf get_global_max();
    vecrf get_global_available();

    void zero_local_containers();
    void zero_global_containers();

    void distribute_fractionally_globally(const vecrf& amount);
    void distribute_fractionally_locally(const vecrf& amount);

    void distribute_lump_locally(const vecrf& amount);

    std::vector<resource_network*> get_all_connected();
    std::set<resource_converter*> get_unique_converters();

    bool processed;
    bool touched; ///for graph operations
};

struct damageable
{
    float health;

    damageable();

    //void damage_fraction(float fraction);
    void damage_amount(float fraction);
    float heal_amount(float frac);

    void reset();

    bool is_alive();

    float get_health_frac();
};

struct suit_part
{
    damageable damage;

    ///between 0 and 1
    ///0 at 50% damage, 1 at 100% damage, currently linear interp
    ///well, really this is environmental normalisation rate
    ///because we don't leak into the environment as such, but both
    ///diffuse into each other with a rate moderated by the pressure difference
    float get_leak_rate();
};

namespace suit_parts
{
    enum suit_parts
    {
        HEAD = 0,
        LSHOULDER,
        LARM,
        LHAND,
        RSHOULDER,
        RARM,
        RHAND,
        CHEST,
        LLEG,
        RLEG,
        LFOOT,
        RFOOT,
        COUNT
    };

    static std::vector<std::string> names
    {
        "Head",
        "Left Shoulder",
        "Left Arm",
        "Left Hand",
        "Right Shoulder",
        "Right Arm",
        "Right Hand",
        "Chest",
        "Left Leg",
        "Right Leg",
        "Left Foot",
        "Right Foot",
        "Error"
    };
}

typedef suit_parts::suit_parts suit_t;

struct suit;

struct suit_status_displayer
{
    renderable_file file;

    suit_status_displayer();

    void tick(state& s, suit& mysuit);
};

///entity or component?
///maybe a suit entity with a suit component?
///seems preposterous that I'm already at the state of
///writing the suit
///so suit needs a backup resource converter piece of storage
///which monitors the atmospheric environment breathed by the player
///and regulates it
///so we want some kind of resource monitor
///where we can set the ideal atmospheric composition
///and then return the difference between the current atmosphere and the ideal
///which we can then attempt to fill in from the suit
///Hydrogen - water - toxic - co2 get stored
///nitrogen is ignored unless its preventing us from adding in oxygen
///ie nitrogen pressure + required oxygen pressure > 1 atm
///oxygen ignored unless too high
///hydrogen, nitrogen, oxygen can be used for backfilling
///order of preference - nitrogen, hydrogen, oxygen
///if we have low nitrogen mix with hydrogen to pevent hydrogen killing you
///and make sure to maintain ratio with oxygen
struct suit
{
    conditional_environment_modifier environment;
    air_displayer display;
    suit_status_displayer suit_display;

    resource_network suit_resource_network;
    environment_balancer* balancer;
    resource_converter resource_storage;
    ///going to need a resource converter later, but for the moment
    ///I just want to test the environmental stacking

    std::map<suit_t, suit_part> parts;

    suit();

    void tick(state&, float dt, vec2f pos);

    float repair(float amount);

    float get_total_leak();
    float get_total_damage();
};

struct mass
{
    float amount;

    mass();

    void set_mass(float _amount);
    float get_velocity_modifier();
};

struct momentum_handler
{
    vec2f velocity;
    mass mymass;

    momentum_handler();
    void set_mass(float _amount);
    vec2f do_movement(state& s, vec2f position, vec2f dir, float dist, float dt, float slowdown_frac);
};

struct repair_component
{
    float repair_remaining;
    repair_component();

    float deplete(float amount);
    float add(float amount);
};



#endif // COMPONENTS_H_INCLUDED
