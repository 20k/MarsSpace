#ifndef GAME_CONSTANTS_H_INCLUDED
#define GAME_CONSTANTS_H_INCLUDED

#include "air.hpp"

///this is going to take a while to fill
namespace game
{
    float player_speed = 21;
    float player_with_suit_speed_modifier = 1.f/1.5f;
    float player_mass = 10.f;
    float player_with_suit_mass = 100.f;

    float interact_distance_from_door = 3;
    float interact_radius_door = 2.f;

    float resource_packet_max_storage = 10.f;
    float resource_packet_default_storage = 10.f;
    float resource_packet_interact_radius = 2.f;

    float solar_panel_watts_ps = 900;

    float hydrogen_battery_watts_storage = 9 * 1000.f * 1000.f;
    float hydrogen_battery_radius = 5.f;
    vec4f hydrogen_battery_colour = (vec4f){255, 140, 0, 255};

    float liquid_gas_storage_litres = 50.f;
    float gas_storage_litres = 50.f * air::liquid_to_gas_conversion_ratio_c02;
    float gas_storage_radius = 5.f;
    vec4f gas_storage_colour = (vec4f){100, 255, 255, 255};
    int  gas_storage_text_size = 24;

};

#endif // GAME_CONSTANTS_H_INCLUDED
