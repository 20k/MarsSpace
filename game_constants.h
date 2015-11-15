#ifndef GAME_CONSTANTS_H_INCLUDED
#define GAME_CONSTANTS_H_INCLUDED

#include "air.hpp"

///this is going to take a while to fill
namespace game
{
    static float player_speed = 21;
    static float player_with_suit_speed_modifier = 1.f/1.5f;
    static float player_mass = 10.f;
    static float player_with_suit_mass = 100.f;

    static float interact_distance_from_door = 3;
    static float interact_radius_door = 2.f;

    static float resource_packet_max_storage = 10.f;
    static float resource_packet_default_storage = 10.f;
    static float resource_packet_interact_radius = 2.f;

    static float solar_panel_watts_ps = 900;
    static float solar_panel_power_storage = 1; ///because otherwise the resources get a bit broken and we cant draw properly if there's no resource storage at all

    static float hydrogen_battery_watts_storage = 9 * 1000.f * 1000.f;
    static float hydrogen_battery_radius = 5.f;
    static vec4f hydrogen_battery_colour = (vec4f){255, 140, 0, 255};

    static float liquid_gas_storage_litres = 50.f;
    static float gas_storage_litres = 50.f * air::liquid_to_gas_conversion_ratio_c02;
    static float gas_storage_radius = 5.f;
    static vec4f gas_storage_colour = (vec4f){100, 255, 255, 255};
    static int   gas_storage_text_size = 24;

    /*float oxygen_reclaimer_litres_ph = 0.5f;
    float oxygen_reclaimer_litres_pm = litres_ph / 60.f;
    float oxygen_reclaimer_litres_ps = litres_pm / 60.f;*/

    ///in terms of gas
    ///this is actually mostly irrelevant, the bottleneck is air transfer
    ///which can be fixed when we gpu process it
    static float oxygen_reclaimer_litres_ps = 0.1;
    static float oxygen_reclaimer_gas_absorbed_ps = 100;
    static float oxygen_reclaimer_co2_storage = 1.f;
    static float oxygen_reclaimer_power_relative = 1000.f; ///that makes the conversion 10000:1 [not a typo]
    static float oxygen_reclaimer_gas_relative = oxygen_reclaimer_litres_ps;

    static float oxygen_reclaimer_radius = 1.f;
    static vec4f oxygen_reclaimer_colour = (vec4f){100, 100, 255, 255};

    static float mining_drill_power_to_iron_ratio = 1.f/4000.f;
    static float mining_drill_iron_storage = 100.f;
    static float mining_drill_iron_kg_ps = 0.1f;

    static float environment_balancer_co2_storage = 0.1f;
    static float environment_balancer_air_processed_ps = 1;
    static float environment_balancer_radius = 1.f;
    static vec4f environment_balancer_colour = (vec4f){255, 100, 255, 255};

    static float resource_filler_radius = 1.f;
    static vec4f resource_filler_colour = (vec4f){128, 128, 128, 255};
    static float resource_filler_air_transfer_ps = 0.1f;
    static vecrf get_ideal_suit_storage();

    static float suit_entity_interact_radius = 2.f;

    static float repair_entity_interact_radius = 1.5f;
    static float repair_entity_repair_speed = 0.1f;

    static float resource_network_effect_radius = 50.f;
    static float resource_network_effect_border = 0.5f;

    static float resource_network_radius = 1.f;
    static float resource_network_border = 1.f;
    static vec4f resource_network_colour = (vec4f){100, 100, 100, 255};

    static float wall_segment_segment_work_speed = 1.f;
    static float wall_segment_segment_interact_distance = 3.f;
    static float wall_segment_segment_interact_rad = 1.5;
    static float wall_segment_segment_wall_work = 1.f; ///...

    static float wall_segment_segment_length = 5.f;
    static float wall_segment_segment_iron_needed = 0.25f; ///this should really be resource required

    static vec4f wall_segment_segment_not_completed_col = (vec4f){255, 140, 140, 255};
    static vec4f wall_segment_segment_completed_col = (vec4f){140, 255, 140, 255};

    static float wall_segment_thickness = 0.5f;
    static float wall_segment_outline_thickness = 0.25f;
    static vec4f wall_segment_colour = (vec4f){190, 190, 190, 255};

    static float breather_lung_air_volume = 0.06f;


    static float health_to_leak_conversion = 0.1f;
    ///this means with a leak rate of 1, we'll equalise pressure with the outside world in 1 second
    static float leak_to_pressure_normalisation_fraction = 0.01f;

    static vecrf get_suit_resource_max_storage();
    static vecrf get_suit_init_storage();
    static vecrf get_suit_init_environment();
    static vecrf get_suit_ideal_environment();
};

vecrf game::get_ideal_suit_storage()
{
    vecrf ideal_suit_storage = 0.f;
    ideal_suit_storage.v[air::OXYGEN] = 1.f;
    ideal_suit_storage.v[air::NITROGEN] = 1.f;

    return ideal_suit_storage;
}

vecrf game::get_suit_resource_max_storage()
{
    vecrf rmax = 0.f;

    rmax.v[air::C02] = 1.f;
    rmax.v[air::OXYGEN] = 1.f;
    rmax.v[air::NITROGEN] = 1.f;

    return rmax;
}

vecrf game::get_suit_init_storage()
{
    vecrf rlocal = 0.f;

    rlocal.v[air::C02] = 0.f;
    rlocal.v[air::OXYGEN] = 1.f;
    rlocal.v[air::NITROGEN] = 1.f;

    return rlocal;
}

vecrf game::get_suit_init_environment()
{
    vecrf ev = 0.f;

    ev.v[air::C02] = 0.f;
    ev.v[air::OXYGEN] = 1.f;
    ev.v[air::NITROGEN] = 0.f;

    return ev;
}

vecrf game::get_suit_ideal_environment()
{
    vecrf ev = 0.f;

    ev.v[air::OXYGEN] = 0.2f;
    ev.v[air::NITROGEN] = 0.8f;

    return ev;
}


#endif // GAME_CONSTANTS_H_INCLUDED
