#ifndef COMPONENTS_H_INCLUDED
#define COMPONENTS_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>

///this project is an experiment in completely modularising all functionality
///into very tiny components, then embedding them into entities
///to see how that plays out
///this is very divergent from how i normally code

struct state
{
    sf::RenderWindow* win;

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
    void tick(state& s, vec2f pos);
};

///this may look like the worst class in the entire universe, but i guarantee you itll be more useful later
struct moveable
{
    vec2f tick(vec2f position, vec2f dir, float dist);
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

#endif // COMPONENTS_H_INCLUDED
