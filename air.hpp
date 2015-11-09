#ifndef AIR_HPP_INCLUDED
#define AIR_HPP_INCLUDED

#include <vec/vec.hpp>

namespace air
{
    ///we could include argon, but its basically the same as nitrogen
    ///we're going to have to model temperature sooner or later
    ///modelling it as a gas would probably work just fine
    ///if we just define LOX, LN2, and LH2O and LC02, then we can
    ///define resource conversions between them and fix the gas problem
    ///or maybe we need to define temperature and auto do state management
    ///that would be awfully convenient
    enum resource : uint8_t
    {
        HYDROGEN = 0,
        NITROGEN,
        OXYGEN,
        C02,
        WATER,
        TOXIC,
        //TEMPERATURE,
        POWER,
        FABRIC ///or some building material of some description ///defined in m2
        ///@TODO: add glue here
    };

    ///this is how many air types there are, ie we go from hydrogen to toxic
    ///but they're all resources
    static constexpr uint8_t COUNT = TOXIC + 1;
    static constexpr uint8_t RES_COUNT = FABRIC + 1;

    static std::vector<std::string> names
    {
        "HYDROGEN",
        "NITROGEN",
        "OXYGEN",
        "CARBON DIOXIDE",
        "WATER",
        "TOXIC",
        //"TEMPERATURE",
        "POWER",
        "FABRIC",
        "ERROR"
    };

    static std::vector<std::string> short_names
    {
        "H2",
        "N2",
        "O2",
        "C02",
        "H20",
        "BAD",
        //"TEMP",
        "PWR",
        "FABR",
        "ERR"
    };

    ///need to use this when converting back and forth
    ///this is why all the numbers are funny
    static float liquid_to_gas_conversion_ratio_oxygen = 861;
    static float liquid_to_gas_conversion_ratio_c02 = 845;
}

typedef air::resource air_t;
typedef air::resource resource_t;

typedef vec<air::RES_COUNT, float> vecrf;
typedef vec<air::COUNT, float> vecair;

inline
vecrf air_to_resource(vec<air::COUNT, float> ac)
{
    vecrf res;
    res = 0.f;

    for(int i=0; i<air::COUNT; i++)
    {
        res.v[i] = ac.v[i];
    }

    return res;
}

inline
vec<air::COUNT, float> resource_to_air(vecrf res)
{
    vec<air::COUNT, float> ret;

    ret = 0.f;

    for(int i=0; i<air::COUNT; i++)
    {
        ret.v[i] = res.v[i];
    }

    return ret;
}

namespace resource = air;

inline
vec<air::COUNT, float> get_martian_atmosphere()
{
    vec<air::COUNT, float> ret = (vec<air::COUNT, float>)
    {
        0,
        2.7 + 1.6, ///might as well model argon as nitrogen
        0.13,
        95.32,
        0.01,
        0/*,
        218 * 100 / 0.01f*/
    };

    ret = ret / (100.f);

    const float mars_atmospheric_thickness = 0.01f;

    ret = ret * mars_atmospheric_thickness;

    return ret;
}

inline
vec<air::COUNT, float> get_earth_atmosphere()
{
    vec<air::COUNT, float> urf = (vec<air::COUNT, float>)
    {
        0.f,
        78.084f + 0.9340f,
        20.946,
        0.0397,
        2.5,
        0.f/*,
        287 * 100*/
    };

    return urf / 100.f;
}

inline
vec<air::COUNT, float> get_controlled_environment_atmosphere()
{
    vec<air::COUNT, float> ret = (vec<air::COUNT, float>)
    {
        0.f,
        78.084f + 0.9340f,
        20.946,
        0.f,
        1,
        0.f/*,
        294 * 100*/
    };

    return ret / 100.f;
}

struct state;

///I'm going to do this the inefficient way :[
///intially assume the surface of mars is a vacuum
///but later we'll want to model the actual crap
struct air_processor
{
    static constexpr int N = air::COUNT;

    static vec<N, float> martian_atmosphere;// = martian_atmosphere();

    ///from percentage to 1, then 1 atmosphere -> 0.01

    vec<N, float>* buf;
    vec<N, float>* buf_out;

    int width, height;

    void load(int _width, int _height);

    ///do a float version of the below that does backwards bilinear interpolation
    ///on the value
    ///so that its smooth as something very smooth
    void add(int x, int y, float amount, air_t type);
    float take(int x, int y, float amount, air_t type);

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
