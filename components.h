#ifndef COMPONENTS_H_INCLUDED
#define COMPONENTS_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>
#include <vector>
#include <memory>
#include "air.hpp"

///this project is an experiment in completely modularising all functionality
///into very tiny components, then embedding them into entities
///to see how that plays out
///this is very divergent from how i normally code

struct entity;
struct movement_blocker;

struct state
{
    sf::RenderWindow* win;
    ///also need movement blockers in here
    std::vector<std::shared_ptr<movement_blocker>> blockers;

    ///or maybe I should make this a std vector for genericness?
    ///use for activating stuff
    entity* current_player;

    sf::Texture planet_tex;

    air_processor* air_process = nullptr;

    state(sf::RenderWindow* _win, sf::Texture&, air_processor&);
};

struct renderable_file
{
    sf::Image img;
    sf::Texture tex;

    void load(const std::string&Y);
    void tick(state& s, vec2f pos);
};

struct renderable_texture
{
    sf::Texture tex;

    void load(sf::Texture& _tex);
    void tick(state& s, vec2f pos);
};

struct renderable_circle
{
    void tick(state& s, vec2f pos, float rad);
};

struct renderable_rectangle
{
    void tick(state& s, vec2f start, vec2f finish, float thickness);
};

///this may look like the worst class in the entire universe, but i guarantee you itll be more useful later
///gets blocked by stuff
struct moveable
{
    vec2f tick(state& s, vec2f position, vec2f dir, float dist);
};

struct keyboard_controller
{
    vec2f tick(float dt);
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

struct wall_segment
{
    movement_blocker block;
    renderable_rectangle rect;

    vec2f start, finish;

    wall_segment(vec2f _start, vec2f _finish);
    void tick(state& s);
};

struct mouse_fetcher
{
    vec2f get_world(state& s);
    vec2f get_screen(state& s);
};

struct area_interacter
{
    renderable_circle circle;
    vec2f pos;
    float radius;

    ///maybe remove _pos and radius from the constructor, as we might want to
    ///interact with moving entities (eg get in a sliding rover)
    area_interacter(vec2f _pos, float _radius);

    void tick(state& s);

    bool player_inside(state& s);
    bool player_has_interacted(state& s); ///will only fire once per button hold

private:
    bool just_interacted;
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

struct saver
{
    void save_to_file(const std::string& fname, const std::vector<entity*> stuff);
    std::vector<entity*> load_from_file(const std::string& fname, state& s);
};

struct text
{
    std::string str;
    vec2f tl;

    ///need to load then render really
    void render(state& s, const std::string& _str, vec2f _tl, int size = 16);
};

struct air_monitor
{
    vec<air::COUNT, float> get_air_fractions(state& s, vec2f pos);

    float get_air_pressure(state& s, vec2f pos);
};

struct air_displayer
{
    text txt;
    air_monitor air_quality;

    void tick(state& s, vec2f pos, vec2f display_pos);
};

struct environmental_gas_emitter
{
    void tick(state& s, vec2f pos, float amount, air::air type);
};

#endif // COMPONENTS_H_INCLUDED
