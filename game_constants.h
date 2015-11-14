#ifndef GAME_CONSTANTS_H_INCLUDED
#define GAME_CONSTANTS_H_INCLUDED

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
};

#endif // GAME_CONSTANTS_H_INCLUDED
