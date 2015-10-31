#ifndef COMPONENTS_H_INCLUDED
#define COMPONENTS_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>
#include <vector>
#include <memory>

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

    state(sf::RenderWindow* _win);
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

    movement_blocker(state& _s, vec2f _start, vec2f _finish);
    ~movement_blocker();

    uint32_t id;
    static uint32_t gid;

    state* s = nullptr;
    std::shared_ptr<movement_blocker> remote;
};

struct wall_segment
{
    movement_blocker block;
    renderable_rectangle rect;

    vec2f start, finish;

    wall_segment(state& s, vec2f _start, vec2f _finish);
    void tick(state& s);
};

struct mouse_fetcher
{
    vec2f get(state& s);
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

#endif // COMPONENTS_H_INCLUDED
