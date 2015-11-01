#ifndef AIR_HPP_INCLUDED
#define AIR_HPP_INCLUDED

#include <vec/vec.hpp>

namespace air
{
    ///we could include argon, but its basically the same as nitrogen
    enum air : uint8_t
    {
        HYDROGEN = 0,
        NITROGEN,
        OXYGEN,
        C02,
        WATER,
        TOXIC,
        COUNT
    };

    static std::vector<std::string> names
    {
        "HYDROGEN",
        "NITROGEN",
        "OXYGEN",
        "CARBON DIOXIDE",
        "WATER",
        "TOXIC",
        "ERROR"
    };
}

struct state;

///I'm going to do this the inefficient way :[
///intially assume the surface of mars is a vacuum
///but later we'll want to model the actual crap
struct air_processor
{
    static constexpr int N = air::COUNT;

    vec<N, float>* buf;
    int width, height;

    void load(int _width, int _height);

    ///do a float version of the below that does backwards bilinear interpolation
    ///on the value
    ///so that its smooth as something very smooth
    void add(int x, int y, float amount, air::air type);
    float take(int x, int y, float amount, air::air type);

    vec<N, float> take_volume(int x, int y, float amount);
    void add_volume(int x, int y, const vec<N, float>& amount);

    ///it is wildly inefficient to do this per frame
    ///or not, we have hardly any actual lines
    void draw_lines(state& s);

    ///this is going to have to be gpu ported
    ///luckily not too impossibru
    void tick(state& s, float dt);
    void draw(state& s);

    vec<N, float> get(float x, float y);

    void save_to_file(const std::string& fname);
    void load_from_file(const std::string& fname);
};

#endif // AIR_HPP_INCLUDED
