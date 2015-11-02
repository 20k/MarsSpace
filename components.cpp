#include "components.h"
#include "entities.h"
#include <fstream>
#include <ostream>
#include <gl/gl.h>
#include <gl/glext.h>
#include <gl/glcorearb.h>

uint32_t movement_blocker::gid = 0;

state::state(sf::RenderWindow* _win, sf::Texture& tex, air_processor& _air)
{
    win = _win;
    planet_tex = tex;

    current_player = nullptr;

    air_process = &_air;
}

void renderable_file::load(const std::string& name)
{
    img.loadFromFile(name.c_str());
    tex.loadFromImage(img);
    tex.setSmooth(true);
    //sf::Texture::bind(&tex);
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
}

void renderable_file::tick(state& s, vec2f pos, float scale)
{
    sf::Sprite spr;
    spr.setTexture(tex);

    int width = tex.getSize().x;
    int height = tex.getSize().y;

    float xp = pos.v[0];
    float yp = pos.v[1];

    spr.setOrigin(width/2.f, height/2.f);
    spr.setPosition(xp, yp);
    spr.setScale(scale, scale);

    s.win->draw(spr);
}

void renderable_texture::load(sf::Texture& _tex)
{
    tex = _tex;
}

void renderable_texture::tick(state& s, vec2f pos)
{
    sf::Sprite spr;
    spr.setTexture(tex);

    s.win->draw(spr);
}

void renderable_circle::tick(state& s, vec2f pos, float rad, vec4f col)
{
    sf::CircleShape circle;
    circle.setOrigin(rad, rad);
    circle.setRadius(rad);

    circle.setPosition(pos.v[0], pos.v[1]);

    circle.setFillColor(sf::Color(col.v[0], col.v[1], col.v[2], col.v[3]));
    circle.setOutlineThickness(0.5f);
    circle.setOutlineColor(sf::Color(col.v[0], col.v[1], col.v[2], col.v[3]/2.f));

    s.win->draw(circle);
}

void renderable_rectangle::tick(state& s, vec2f start, vec2f finish, float width)
{
    vec2f diff = finish - start;

    float angle = diff.angle();
    float len = diff.length();

    sf::RectangleShape rect;
    rect.setSize(sf::Vector2f(len, width));

    rect.setPosition({start.v[0], start.v[1]});
    rect.setOrigin(0.f, width/2.f);

    rect.setRotation(angle*360/(2*M_PIf));
    rect.setFillColor(sf::Color(190, 190, 190));

    rect.setOutlineThickness(0.5);
    rect.setOutlineColor(sf::Color(110,110,110, 255));

    s.win->draw(rect);
}

///maybe we want a collider that will return a collision and an optional movement vector
///then we can use this for cars as well
///we're no longer using the vector collision system
///this means that things going too fast can skip throuhg the wall
///however it has the capacity to be upgraded to handle that, so no major concern
vec2f moveable::tick(state& s, vec2f position, vec2f dir, float dist)
{
    if(dist < 0.001f)
        return position;

    vec2f new_pos = position + dir.norm() * dist;

    for(auto& i : s.blockers)
    {
        if(i->start == i->finish)
            continue;

        //bool s1 = is_left_side(i->start, i->finish, position);
        //bool s2 = is_left_side(i->start, i->finish, new_pos);

        vec2f avg = (i->start + i->finish) / 2.f;

        ///padme?
        float rad = (i->finish - i->start).length() / 2.f;
        float rpad = 1.f;

        float dist_to_line_centre = (new_pos - avg).length();

        vec2f to_wall = point2line_shortest(i->start, (i->finish - i->start), new_pos);

        const float pad = 2.f;

        float to_length = to_wall.length();

        float f1l = (new_pos - i->start).length();
        float f2l = (new_pos - i->finish).length();

        if(dist_to_line_centre > rad && dist_to_line_centre <= rad + rpad)
        {
            if(f1l < f2l)
                to_wall = -(new_pos - i->start);
            else
                to_wall = -(new_pos - i->finish);

            to_length = to_wall.length();
        }

        if(to_length < pad && dist_to_line_centre <= rad + rpad)
        {
            if(to_wall.v[0] == 0 && to_wall.v[1] == 0)
            {
                to_wall = (position - avg).norm();

                ///this is really very degenerate to the point where I'm not sure its possible
                ///for the moment just move the player diagonally
                if(to_wall.v[0] == 0 && to_wall.v[1] == 0)
                {
                    to_wall.v[0] = 1;
                    to_wall.v[1] = 1;
                }
            }

            new_pos = new_pos + -to_wall.norm() * (pad - to_length);

            //s2 = is_left_side(i->start, i->finish, new_pos);
        }
    }

    return new_pos;
}

vec2f keyboard_controller::tick(float dt)
{
    sf::Keyboard key;

    vec2f dir = (vec2f){0, 0};

    if(key.isKeyPressed(sf::Keyboard::W))
        dir.v[1] += -1.f;
    if(key.isKeyPressed(sf::Keyboard::S))
        dir.v[1] += 1.f;

    if(key.isKeyPressed(sf::Keyboard::A))
        dir.v[0] += -1.f;
    if(key.isKeyPressed(sf::Keyboard::D))
        dir.v[0] += 1.f;

    return dir * dt;
}

void speed_handler::set_speed(float _speed)
{
    speed = _speed;
}

float speed_handler::get_speed()
{
    return speed;
}

///we need to fix the movement blockers memory ownership
movement_blocker::movement_blocker(vec2f _start, vec2f _finish)
{
    start = _start;
    finish = _finish;

    id = gid++;

    printf(":)\n");

    remote = nullptr;
}

void movement_blocker::push_remote(state& s)
{
    remote = std::make_shared<movement_blocker>(*this);

    s.blockers.push_back(remote);
}

///@TODO: This is stupid, FIXME
void movement_blocker::destroy_remote(state& s)
{
    remote = nullptr;

    for(int i=0; i<(int)s.blockers.size(); i++)
    {
        ///I'm the last
        ///wait, if its a shared pointer it owns itself as wlel
        ///so this is 2
        ///but that means if im deconstructed and then reconstructed
        ///then broken
        ///so, rip in peace for the moment
        if(s.blockers[i].use_count() == 2)
        {
            printf(":(\n");

            s.blockers.erase(s.blockers.begin() + i);
        }
    }
}

void movement_blocker::tick(state& s)
{
    if(remote == nullptr)
    {
        push_remote(s);
    }
}

void movement_blocker::modify_bounds(vec2f _start, vec2f _finish)
{
    start = _start;
    finish = _finish;

    if(remote)
    {
        remote->start = _start;
        remote->finish = _finish;
    }
}

wall_segment::wall_segment(vec2f _start, vec2f _finish) : block(_start, _finish)
{
    start = _start;
    finish = _finish;
}

void wall_segment::tick(state& s)
{
    rect.tick(s, start, finish, 1.f);

    block.tick(s);
}

vec2f mouse_fetcher::get_world(state& s)
{
    sf::Mouse mouse;

    int x = mouse.getPosition(*s.win).x;
    int y = mouse.getPosition(*s.win).y;

    auto mouse_pos = s.win->mapPixelToCoords({x, y});

    return {mouse_pos.x, mouse_pos.y};
}

vec2f mouse_fetcher::get_screen(state& s)
{
    sf::Mouse mouse;

    float x = mouse.getPosition(*s.win).x;
    float y = mouse.getPosition(*s.win).y;

    return {x, y};
}

area_interacter::area_interacter(vec2f _pos, float _radius)
{
    pos = _pos;
    radius = _radius;
    just_interacted = false;
}

void area_interacter::tick(state& s)
{
    circle.tick(s, pos, radius, (vec4f){220, 200, 200, 100});
}

bool area_interacter::player_inside(state& s)
{
    if(s.current_player == nullptr)
        return false;

    vec2f player_pos = s.current_player->position;

    vec2f rel = player_pos - pos;

    float dist = rel.length();

    if(dist < radius)
    {
        return true;
    }

    return false;
}

bool area_interacter::player_has_interacted(state& s)
{
    if(!player_inside(s))
        return false;

    ///need to pull this logic out into a separate component
    sf::Keyboard key;

    if(key.isKeyPressed(sf::Keyboard::E))
    {
        if(!just_interacted)
        {
            just_interacted = true;
            return true;
        }
    }
    else
    {
        just_interacted = false;
    }

    return false;
}


opener::opener(float _time)
{
    time_duration = _time;
    open_frac = 0.f;
    direction = 0;
}

void opener::open()
{
    ///if we're anything other than open
    ///start opening
    if(open_frac < 1.f)
        direction = 1;
    else
        direction = 0;
}

void opener::close()
{
    if(open_frac > 0.f)
        direction = -1;
    else
        direction = 0;
}

void opener::toggle()
{
    if(direction != 0)
        direction = -direction;

    if(direction == 0)
    {
        if(open_frac >= 1.f)
            close();
        if(open_frac <= 0.f)
            open();
    }
}

float opener::get_open_fraction()
{
    return open_frac;
}

void opener::tick(float dt)
{
    open_frac += dt * direction / time_duration;

    if(open_frac >= 1.f || open_frac <= 0)
        direction = 0;

    open_frac = clamp(open_frac, 0.f, 1.f);
}

vec2f squasher::get_squashed_end(vec2f start, vec2f finish, float squash_fraction)
{
    return mix(start, finish, squash_fraction);
}

void saver::save_to_file(const std::string& fname, const std::vector<entity*> stuff)
{
    FILE* pFile = fopen(fname.c_str(), "wb");

    for(auto& i : stuff)
    {
        save s = i->make_save();

        ///keeping type separate from the rest of it because its information for the saver
        ///not information for saved class
        //fprintf(pFile, "%i", s.type);

        fwrite(&s.type, sizeof(s.type), 1, pFile);

        auto vec = s.vec;
        auto ptr = vec.data();

        if(ptr.size() > 0)
            fwrite(&ptr[0], ptr.size(), 1, pFile);
    }

    fclose(pFile);
}

std::vector<entity*> saver::load_from_file(const std::string& fname, state& s)
{
    s.blockers.clear();
    s.current_player = nullptr;

    std::string contents;

    std::ifstream in(fname, std::ios::in | std::ios::binary);
    if (in)
    {
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }

    if(contents.size() == 0)
        return std::vector<entity*>();

    byte_fetch fetch;
    fetch.push_back(contents);

    std::vector<entity*> entities;

    while(fetch.valid())
    {
        entity_t type = fetch.get<entity_t>();

        entity* ent;

        if(type == entity_type::PLAYER)
        {
            ent = new player(fetch, s);
        }
        else if(type == entity_type::PLANET)
        {
            ent = new planet(fetch, s);
        }
        else if(type == entity_type::BUILDING)
        {
            ent = new building(fetch, s);
        }
        else if(type == entity_type::DOOR)
        {
            ent = new door(fetch);
        }
        else if(type == entity_type::RESOURCE_ENTITY)
            ent = new resource_entity(fetch);

        else if(type == entity_type::SOLAR_PANEL)
            ent = new solar_panel(fetch);

        else if(type == entity_type::HYDROGEN_BATTERY)
            ent = new hydrogen_battery(fetch);

        else if(type == entity_type::GAS_STORAGE)
            ent = new gas_storage(fetch);

        else if(type == entity_type::OXYGEN_RECLAIMER)
            ent = new oxygen_reclaimer(fetch);

        entities.push_back(ent);
    }

    return entities;
}

void text::render(state& s, const std::string& _str, vec2f _tl, int size, text_options::text_options opt)
{
    str = _str;
    tl = _tl;

    static bool loaded = false;

    static sf::Font font;
    static sf::Text txt;

    if(!loaded)
    {
        font.loadFromFile("./VeraMono.ttf");
        txt.setFont(font);

        loaded = true;
    }


    txt.setCharacterSize(size);

    //sf::Text txt(str, font, size);
    txt.setString(str);
    txt.setPosition(tl.v[0], tl.v[1]);

    bool is_absolute = opt & text_options::ABS;

    ///bad
    if(!is_absolute)
        txt.setScale(0.1f, 0.1f);
    else
        txt.setScale(1.f, 1.f);

    if(opt & text_options::CENTERED)
    {
        txt.setOrigin({txt.getLocalBounds().width / 2.f, txt.getLocalBounds().height / 1.2f});
    }
    else
        txt.setOrigin({0.f, 0.f});

    /*if(!is_absolute)
        s.win->draw(txt);
    else
    {
        sf::View cur = s.win->getView();
        sf::View def = s.win->getDefaultView();
        s.win->setView(def);
        s.win->draw(txt);
        s.win->setView(cur);
    }*/

    sf::View cur = s.win->getView();

    if(is_absolute)
    {
        sf::View def = s.win->getDefaultView();
        s.win->setView(def);
    }

    /*if(opt & text_options::OUTLINED)
    {
        for(int j=-1; j<=1; j++)
        {
            for(int i=-1; i<=1; i++)
            {
                auto pos = txt.getPosition();

                pos.x = pos.x + i;
                pos.y = pos.y + i;

                txt.setPosition(pos);
                s.win->draw(txt);
            }
        }
    }
    else
    {*/
    s.win->draw(txt);
    /*}*/

    if(is_absolute)
    {
        s.win->setView(cur);
    }
}

vec<air::COUNT, float> air_monitor::get_air_fractions(state& s, vec2f pos)
{
    auto vec = s.air_process->get(pos.v[0], pos.v[1]);

    return vec / vec.sum();
}

vec<air::COUNT, float> air_monitor::get_air_parts(state& s, vec2f pos)
{
    auto vec = s.air_process->get(pos.v[0], pos.v[1]);

    return vec;
}

float air_monitor::get_air_pressure(state& s, vec2f pos)
{
    return s.air_process->get(pos.v[0], pos.v[1]).sum();
}

void air_displayer::tick(state& s, vec2f display_pos, const vec<air::COUNT, float>& air_parts, bool absolute)
{
    //auto air_fracs = air_quality.get_air_fractions(s, pos);
    //float air_pressure = air_quality.get_air_pressure(s, pos);

    auto air_fracs = air_parts / air_parts.sum();
    float air_pressure = air_parts.sum();


    if(air_pressure < 0.00001f || std::isnan(air_pressure))
    {
        air_pressure = 0.f;

        for(int i=0; i<air::COUNT; i++)
        {
            air_fracs.v[i] = 0.f;
        }
    }

    std::string display;

    for(int i=0; i<air::COUNT; i++)
    {
        display = display + air::names[i] + ": " + std::to_string(air_fracs.v[i] * 100.f) + "%" + "\n";
    }

    if(air_pressure > 0.f)
        display = display + "PRESSURE: " + std::to_string(air_pressure);
    else
        display = display + "PRESSURE: TOTAL VACUUM";


    if(!absolute)
        txt.render(s, display, display_pos, 16, text_options::NONE);
    else
        txt.render(s, display, display_pos, 16, text_options::ABS);

}

void resource_displayer::tick(state& s, vec2f display_pos, const vecrf& resources, bool absolute)
{
    auto resource_parts = resources;

    std::string display;

    for(int i=0; i<resource::RES_COUNT; i++)
    {
        //if(resource_parts.v[i] <= 0.f)
        //    continue;

        display = display + resource::names[i] + ": " + std::to_string(resource_parts.v[i]) + "\n";
    }

    if(!absolute)
        txt.render(s, display, display_pos, 16, text_options::NONE);
    else
        txt.render(s, display, display_pos, 16, text_options::ABS);
}

void environmental_gas_emitter::emit(state& s, vec2f pos, float amount, air_t type)
{
    s.air_process->add(pos.v[0], pos.v[1], amount, type);
}

float environmental_gas_absorber::absorb(state& s, vec2f pos, float amount, air_t type)
{
    return s.air_process->take(pos.v[0], pos.v[1], amount, type);
}

air_environment::air_environment()
{
    local_environment = 0.f;
}

///air environment will be the class that can be enclosed
///so later we'll replace with either pull from the environment, or my parent
///works out quite well
void air_environment::absorb_all(state& s, vec2f pos, float amount, float max_total)
{
    /*auto air_parts = monitor.get_air_parts(s, pos);
    float total_available_air = monitor.get_air_pressure(s, pos);

    if(amount < total_available_air)
        amount = total_available_air;*/

    float current_amount = local_environment.sum();

    if(current_amount + amount > max_total)
    {
        amount = max_total - current_amount;
    }

    if(amount < 0)
        amount = 0;

    ///so amount is the volume of air to take in
    ///we simply request volume air from the thing

    auto taken = s.air_process->take_volume(pos.v[0], pos.v[1], amount);


    local_environment = local_environment + air_to_resource(taken);
}

///above also applies to this obvs
void air_environment::emit_all(state& s, vec2f pos, float amount)
{
    float my_air = local_environment.sum();

    if(amount >= my_air)
        amount = my_air;

    auto frac = local_environment / local_environment.sum();

    auto to_emit = frac * amount;

    s.air_process->add_volume(pos.v[0], pos.v[1], resource_to_air(to_emit));

    local_environment = local_environment - to_emit;
}

void air_environment::convert_percentage(float amount, float fraction, air_t input, air_t output)
{
    float in = local_environment.v[input];

    float converted = amount * fraction;

    if(converted > in)
    {
        converted = in;
    }

    local_environment.v[output] += converted;
    local_environment.v[input] -= converted;
}

///wrong
void conditional_environment_modifier::absorb_all(state& s, vec2f pos, float amount)
{
    if(parent == nullptr)
        my_environment.absorb_all(s, pos, amount, max_air);
    else
    {
        vecrf& parent_storage = parent->my_environment.local_environment;
        vecrf& local_storage = my_environment.local_environment;

        ///parent environment is empty, cannot absorb
        if(parent_storage.sum() < 0.0001f)
            return;

        vecrf taken = parent->take(amount);

        vecrf leftover = add(taken);

        parent->add(leftover);
    }
}

///emitting into an environment is the same as that environment taking form me
void conditional_environment_modifier::emit_all(state& s, vec2f pos, float amount)
{
    if(parent == nullptr)
    {
        my_environment.emit_all(s, pos, amount);
    }
    else
    {
        auto backup_parent = parent;

        parent->parent = this;

        parent->absorb_all(s, pos, amount);

        parent->parent = backup_parent;
    }
}

vecrf conditional_environment_modifier::take(float amount)
{
    float amount_in_storage = my_environment.local_environment.sum();

    if(amount_in_storage < 0.0001f)
    {
        vecrf z;
        z = 0.f;
        return z;
    }

    if(amount > amount_in_storage)
        amount = amount_in_storage;

    auto ret_amount = (my_environment.local_environment / amount_in_storage) * amount;

    my_environment.local_environment = my_environment.local_environment - ret_amount;

    return ret_amount;
}

vecrf conditional_environment_modifier::add(vecrf amount)
{
    float total = amount.sum() + my_environment.local_environment.sum();

    if(total < 0.0001f || amount.sum() < 0.0001f)
    {
        vecrf z;
        z = 0.f;
        return z;
    }

    float to_add = total;

    if(to_add > max_air)
        to_add = max_air;

    float just_amount_extra = to_add - my_environment.local_environment.sum();

    auto ret = just_amount_extra * amount / amount.sum();

    my_environment.local_environment = my_environment.local_environment + ret;

    return amount - ret;
}

void conditional_environment_modifier::set_max_air(float _max)
{
    max_air = _max;
}

void conditional_environment_modifier::set_parent(conditional_environment_modifier* _parent)
{
    parent = _parent;
}

void conditional_environment_modifier::remove_parent()
{
    parent = nullptr;
}

///i think this assumes 1x1 square tiles
void breather::tick(state& s, vec2f position, float dt)
{
    ///lets put this into a breathing manager afterwards
    float breaths_per_minute = 14;

    float breaths_per_second = breaths_per_minute / 60.f;

    //float breaths_per_ms = breaths_per_second / 1000.f;

    float ldt = dt * breaths_per_second;

    ///use this for some sort of hardcore realism mode where it takes 2 real days time.
    ///or maybe we can just accelerate time?
    const float volume_per_breath_m3 = 0.0031;
    const float volume_per_breath_litres = 6;


    ///assume 1 is atmospheric pressure
    ///then model lung volume breathing n stuff
    lungs.set_max_air(1);
    lungs.absorb_all(s, position, volume_per_breath_litres * 1.f * ldt);
    display.tick(s, (vec2f){20.f, 20.f}, resource_to_air(lungs.my_environment.local_environment), true);

    ///0.2f because we've inhaled an ideal 1 atm of air
    ///0.2% idealls is o2
    ///0.05 / 0.2 of which is converted to c02 in the ideal case
    lungs.my_environment.convert_percentage(volume_per_breath_litres * 0.2f * ldt, 0.05f / 0.20f, air::OXYGEN, air::C02);

    display.tick(s, (vec2f){20.f, 200.f}, resource_to_air(lungs.my_environment.local_environment), true);

    lungs.emit_all(s, position, 2.f);
}

resource_converter::resource_converter()
{
    for(int i=0; i<resource::RES_COUNT; i++)
    {
        local_storage.v[i] = 0;
        max_storage.v[i] = 0;

        conversion_usage_ratio.v[i] = 0;
        conversion_output_ratio.v[i] = 0;
    }

    amount = 0.f;
    efficiency = 1.f;
    environmental_absorption_rate = 0.f;
    pos = (vec2f){0.f, 0.f};
}

void resource_converter::set_max_storage(const std::vector<std::pair<resource_t, float>>& lv)
{
    for(auto& i : lv)
    {
        max_storage.v[i.first] = i.second;
    }
}

void resource_converter::set_usage_ratio(const std::vector<std::pair<resource_t, float>>& lv)
{
    for(auto& i : lv)
    {
        conversion_usage_ratio.v[i.first] = i.second;
    }

    conversion_usage_ratio = conversion_usage_ratio / conversion_usage_ratio.sum();
}

void resource_converter::set_output_ratio(const std::vector<std::pair<resource_t, float>>& lv)
{
    for(auto& i : lv)
    {
        conversion_output_ratio.v[i.first] = i.second;
    }

    conversion_output_ratio = conversion_output_ratio / conversion_output_ratio.sum();
}

void resource_converter::set_absorption_rate(float _rate)
{
    environmental_absorption_rate = _rate;
}

void resource_converter::add(const std::vector<std::pair<resource_t, float>>& lv)
{
    for(auto& i : lv)
    {
        local_storage.v[i.first] += i.second;
    }

    vec<resource::RES_COUNT, float> minimum;
    minimum = 0.f;

    local_storage = clamp(local_storage, minimum, max_storage);
}

vec<resource::RES_COUNT, float> resource_converter::take(const std::vector<std::pair<resource_t, float>>& lv)
{
    auto old = local_storage;

    for(auto& i : lv)
    {
        local_storage.v[i.first] -= i.second;
    }

    vec<resource::RES_COUNT, float> minimum;
    minimum = 0.f;

    local_storage = clamp(local_storage, minimum, max_storage);

    return old - local_storage;
}

/*void resource_converter::add(const vecrf& v)
{
    local_storage = local_storage + v;

    local_storage = max(local_storage, 0.f);
    local_storage = min(local_storage, max_storage);
}*/

///we want to deplete from local resources first, then escalate to global resources
///this is so that we can absorb a unit of gas from the environment, process it, then re-emit it afterwards
void convert_amount(float amount, vecrf& global_storage, vecrf& global_max, vecrf& local_environment, float dt, float efficiency, const vecrf& conversion_usage_ratio, const vecrf& conversion_output_ratio)
{
    if(amount <= 0.001f)
        return;

    ///so we steal resources from the local environment (ie what we've deliberately absorbed)
    auto resources = global_storage + local_environment;

    auto consume_amount = conversion_usage_ratio * amount * dt;

    auto diff = resources - consume_amount;

    auto minimum_element = diff.min_elem();

    ///we've requested an extra x too much of a particular resources
    ///we'll have to request minimum_element * conversion_usage_ratio of everything
    ///or... we could just recurse
    if(minimum_element < -0.00001f)
    {
        float amount_too_much_element = fabs(minimum_element) / dt;
        amount_too_much_element /= conversion_usage_ratio.v[diff.which_element_minimum()];

        //std::cout << conversion_usage_ratio << std::endl;
        //printf("throttlin %f %f %f\n", minimum_element, amount_too_much_element, dt);

        //printf("Not enough %s\n", air::names[diff.which_element_minimum()].c_str());

        return convert_amount(std::max(amount - amount_too_much_element, 0.f), global_storage, global_max, local_environment, dt, efficiency, conversion_usage_ratio, conversion_output_ratio);
    }

    auto amount_produced = amount * dt * efficiency * conversion_output_ratio;

    auto pre = local_environment;

    local_environment = max(local_environment - consume_amount, 0.f);

    auto local_diff = pre - local_environment;

    global_storage = global_storage - consume_amount;
    global_storage = global_storage + amount_produced + local_diff;
    ///so local diff is the amount that was taken from the local environment
    ///if ie global storage = 0
    ///we consume consume_amount (which will be the maximum we can take from the local environment
    ///remove that from global storage, then readd the local diff
    ///=== 0 change overall

    ///we want to return the resources used if we're above global max
    ///EREGEHRGH
    global_storage = min(global_storage, global_max);
    global_storage = max(global_storage, 0.f);

    ///so we remove the whole consumed amount from the environment
    ///because its not a store, its just a temporary thing
    ///so we definitely remove the resources we can have from there
    ///eg we take all the nitrogen we can from it, there's no 'store'
    ///we can't accidentally remove a resource we don't want to because we always
    ///want to take from the local environment
}

///currently if i've run out of one input resource, itll still work
void resource_converter::convert(vecrf& global_storage, vecrf& global_max, float dt)
{
    convert_amount(amount, global_storage, global_max, environment_absorption.local_environment, dt, efficiency, conversion_usage_ratio, conversion_output_ratio);
}

void resource_converter::set_position(vec2f _pos)
{
    pos = _pos;
}

void resource_converter::absorb_all(state& s, float dt)
{
    if(environmental_absorption_rate > 0.f)
    {
        environment_absorption.absorb_all(s, pos, environmental_absorption_rate * dt, environmental_absorption_rate);

        //printf("%f\n", environmental_absorption_rate * dt);
    }
}

void resource_converter::emit_all(state& s)
{
    if(environmental_absorption_rate > 0.f)
    {
        environment_absorption.emit_all(s, pos, environment_absorption.local_environment.sum());
    }
}

void resource_converter::set_amount(float _amount)
{
    amount = _amount;
}

void resource_converter::set_efficiency(float _efficiency)
{
    efficiency = _efficiency;
}

resource_network::resource_network()
{
    network_resources = 0.f;
}

void resource_network::add(resource_converter* conv)
{
    network_resources = network_resources + conv->local_storage;
    converters.push_back(conv);
}

void resource_network::rem(resource_converter* conv)
{
    network_resources = network_resources - conv->local_storage;

    for(int i=0; i<converters.size(); i++)
    {
        if(converters[i] == conv)
        {
            converters.erase(converters.begin() + i);
            i--;
        }
    }
}

void resource_network::clear()
{
    network_resources = 0;
    converters.clear();
}

///no processing done here
///only resource distribution
///distribute equally? or proportionally?
///in the long term we'll want to set up container priorities
///we also need to be able to extract from the network
///proportional is easiest
///I'm going to have to scrap this and do it properly, aren't I
void resource_network::tick(state& s, float dt)
{
    if(converters.size() == 0)
        return;

    //vecrf resource_accum;
    //resource_accum = 0.f;

    max_network_resources = 0.f;

    for(auto& i : converters)
    {
        //resource_accum = resource_accum + i->local_storage;
        max_network_resources = max_network_resources + i->max_storage;
    }

    for(auto& i : converters)
    {
        i->absorb_all(s, dt);
        //printf("%f\n", i->environment_absorption.local_environment.v[air::C02]);
        i->convert(network_resources, max_network_resources, dt);
        i->emit_all(s);
    }

    ///distribute resources proportionally
    ///if something is destroyed, we'll lose the
    ///proportional resources from the network
    for(auto& c : converters)
    {
        auto storage = c->max_storage;

        vecrf frac_to_store;

        for(int i=0; i<resource::RES_COUNT; i++)
        {
            if(storage.v[i] == 0)
                frac_to_store.v[i] = 0;
            else
            {
                ///give me storage relative to the overall storage
                frac_to_store.v[i] = storage.v[i] / max_network_resources.v[i];
            }
        }

        ///we're doing this for the moment so that later I can affect the lost local storages
        ///now that the network actually stores the resources, this is broken
        auto local_frac = network_resources * frac_to_store;

        c->local_storage = local_frac;
    }


    /*float num = converters.size();

    vecrf avg_per_container = resource_accum / num;
    vecrf running_missed;
    running_missed = 0;*/


    /*for(auto& c : converters)
    {
        auto storage = c->max_storage;

        //auto local = c->local_storage;

        auto fill_amount = avg_per_container + running_missed;

        auto new_local = min(fill_amount, storage);

        //auto change = new_local - local;

        auto extra = avg_per_container - new_local;

        running_missed = running_missed + extra;

        c->local_storage = new_local;
    }*/

    /*for(auto& c : converters)
    {
        auto storage = c->max_storage;

        auto local = c->local_storage;

        auto fill_amount = running_missed;

        auto new_local = min(fill_amount, storage);

        auto change = new_local - local;

        auto extra = avg_per_container - change;

        running_missed = running_missed + extra;

        c->local_storage = new_local;
    }*/
}
