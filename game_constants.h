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
    float solar_panel_power_storage = 1; ///because otherwise the resources get a bit broken and we cant draw properly if there's no resource storage at all

    float hydrogen_battery_watts_storage = 9 * 1000.f * 1000.f;
    float hydrogen_battery_radius = 5.f;
    vec4f hydrogen_battery_colour = (vec4f){255, 140, 0, 255};

    float liquid_gas_storage_litres = 50.f;
    float gas_storage_litres = 50.f * air::liquid_to_gas_conversion_ratio_c02;
    float gas_storage_radius = 5.f;
    vec4f gas_storage_colour = (vec4f){100, 255, 255, 255};
    int   gas_storage_text_size = 24;

    /*float oxygen_reclaimer_litres_ph = 0.5f;
    float oxygen_reclaimer_litres_pm = litres_ph / 60.f;
    float oxygen_reclaimer_litres_ps = litres_pm / 60.f;*/

    ///in terms of gas
    ///this is actually mostly irrelevant, the bottleneck is air transfer
    ///which can be fixed when we gpu process it
    float oxygen_reclaimer_litres_ps = 0.1;
    float oxygen_reclaimer_gas_absorbed_ps = 100;
    float oxygen_reclaimer_co2_storage = 1.f;
    float oxygen_reclaimer_power_relative = 1000.f; ///that makes the conversion 10000:1 [not a typo]
    float oxygen_reclaimer_gas_relative = oxygen_reclaimer_litres_ps;

    float oxygen_reclaimer_radius = 1.f;
    vec4f oxygen_reclaimer_colour = (vec4f){100, 100, 255, 255};

    float mining_drill_power_to_iron_ratio = 1.f/4000.f;
    float mining_drill_iron_storage = 100.f;
    float mining_drill_iron_kg_ps = 0.1f;

    float environment_balancer_co2_storage = 0.1f;
    float environment_balancer_air_processed_ps = 1;
    float environment_balancer_radius = 1.f;
    vec4f environment_balancer_colour = (vec4f){255, 100, 255, 255};

    float resource_filler_radius = 1.f;
    vec4f resource_filler_colour = (vec4f){128, 128, 128, 255};
    float resource_filler_air_transfer_ps = 0.1f;
    static vecrf get_ideal_suit_storage();

    float suit_entity_interact_radius = 2.f;

    float repair_entity_interact_radius = 1.5f;
    float repair_entity_repair_speed = 0.1f;

    float resource_network_effect_radius = 50.f;
    float resource_network_effect_border = 0.5f;

    float resource_network_radius = 1.f;
    float resource_network_border = 1.f;
    vec4f resource_network_colour = (vec4f){100, 100, 100, 255};
};

vecrf game::get_ideal_suit_storage()
{
    vecrf ideal_suit_storage = 0.f;
    ideal_suit_storage.v[air::OXYGEN] = 1.f;
    ideal_suit_storage.v[air::NITROGEN] = 1.f;

    return ideal_suit_storage;
}

#endif // GAME_CONSTANTS_H_INCLUDED
