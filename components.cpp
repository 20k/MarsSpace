#include "components.h"
#include "entities.h"
#include <fstream>
#include <ostream>
#include <gl/gl.h>
#include <gl/glext.h>
#include <gl/glcorearb.h>

#include "game_constants.h"

uint32_t movement_blocker::gid = 0;

///gameplay constants initialised in this file need to be externalised


state::state(sf::RenderWindow* _win, sf::Texture& tex, air_processor& _air)
{
    win = _win;
    planet_tex = tex;

    current_player = nullptr;

    air_process = &_air;

    ///1 is sun from up/right
    sun_direction = (vec2f){0, 1};
}

void renderable_file::load(const std::string& name)
{
    img.loadFromFile(name.c_str());
    tex.loadFromImage(img);
    tex.setSmooth(true);

    rtex = new sf::RenderTexture;
    rtex->create(tex.getSize().x * 4, tex.getSize().y * 4);
    rtex->setSmooth(true);
    //sf::Texture::bind(&tex);
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
}

///actually we can do shadows super easily in 2d top down, we just project and scale the drawing skewed
///now, we need to blur the crap out of the shadows
///we could super downscale them then upscale them?
void renderable_file::tick(state& s, vec2f pos, float scale, float rotation, bool shadow, bool absolute, sf::RenderStates rs)
{
    float xp = pos.v[0];
    float yp = pos.v[1];

    if(shadow)
    {
        rtex->clear(sf::Color(0, 0, 0, 0));

        sf::Sprite spr2;
        spr2.setTexture(tex);

        vec2f sun = s.sun_direction * 20.f;


        ///draw rotated first at offset, then scaleme
        spr2.setOrigin(tex.getSize().x/2.f, tex.getSize().y/2.f);
        spr2.setPosition(rtex->getSize().x/2.f + sun.v[0], rtex->getSize().y/2.f + sun.v[1]);
        spr2.setRotation(r2d(rotation));
        spr2.setColor(sf::Color(0,0,0,128));
        //spr2.setScale(2, 2);

        //s.win->draw(spr2);

        rtex->draw(spr2);
        rtex->display();
    }


    /*spr.setOrigin(width/2.f, height/2.f);
    spr.setPosition(xp, yp);
    spr.setScale(scale, scale);
    spr.setRotation(r2d(rotation));

    spr.setColor(sf::Color(255, 255, 255, 255));
    spr.setScale(scale, scale);
    spr.setPosition(xp, yp);*/

    sf::Sprite rspr;
    rspr.setTexture(rtex->getTexture());
    rspr.setOrigin(rtex->getSize().x/2.f, rtex->getSize().y/2.f);
    rspr.setPosition(xp, yp);
    rspr.setScale(scale, scale);
    //rspr.setRotation(r2d(rotation));

    if(shadow)
        s.win->draw(rspr);

    sf::Sprite spr;
    spr.setTexture(tex);

    //spr.setTexture(rtex->getTexture());
    spr.setOrigin(tex.getSize().x/2.f, tex.getSize().y/2.f);
    spr.setRotation(r2d(rotation));

    spr.setPosition(xp, yp);
    spr.setScale(scale, scale);

    sf::View cur = s.win->getView();

    if(absolute)
    {
        sf::View def = s.win->getDefaultView();

        s.win->setView(def);
    }

    s.win->draw(spr, rs);

    if(absolute)
    {
        s.win->setView(cur);
    }
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

void renderable_circle::tick(state& s, vec2f pos, float rad, vec4f col, float outline_thickness, float outline_factor)
{
    sf::CircleShape circle;
    circle.setOrigin(rad, rad);
    circle.setRadius(rad);

    circle.setPosition(pos.v[0], pos.v[1]);
    circle.setPointCount(100);

    circle.setFillColor(sf::Color(col.v[0], col.v[1], col.v[2], col.v[3]));
    circle.setOutlineThickness(outline_thickness);
    circle.setOutlineColor(sf::Color(col.v[0], col.v[1], col.v[2], col.v[3] * outline_factor));

    s.win->draw(circle);
}

void renderable_rectangle::tick(state& s, vec2f start, vec2f finish, float width, vec4f col, float outline_thickness)
{
    vec2f diff = finish - start;

    col = clamp(col, 0.f, 255.f);

    float angle = diff.angle();
    float len = diff.length();

    sf::RectangleShape rect;
    rect.setSize(sf::Vector2f(len, width));

    rect.setPosition({start.v[0], start.v[1]});
    rect.setOrigin(0.f, width/2.f);

    rect.setRotation(angle*360/(2*M_PIf));
    rect.setFillColor(sf::Color(col.v[0], col.v[1], col.v[2], col.v[3]));

    rect.setOutlineThickness(outline_thickness);
    rect.setOutlineColor(sf::Color(110,110,110, 255));

    s.win->draw(rect);
}

constructable::constructable()
{
    max_work = 0.f;
    achieved_work = 0.f;
}

void constructable::set_work_to_complete(float amount)
{
    max_work = amount;
}

void constructable::apply_work(float amount)
{
    achieved_work += amount;
}

bool constructable::is_constructed()
{
    return achieved_work >= max_work - 0.00001f;
}

float constructable::get_completed_frac()
{
    if(max_work <= 0.f)
        return 1.f;

    return achieved_work / max_work;
}

resource_requirer::resource_requirer()
{
    res_required = 0.f;
    res_added = 0.f;
}

void resource_requirer::set_resource_requried(resource_t type, float max_resource)
{
    res_required.v[type] = max_resource;
}

vecrf resource_requirer::add(const vecrf& res)
{
    auto res_prev = res_added;

    res_added = res_added + res;

    res_added = min(res_added, res_required);

    return res - (res_added - res_prev);
}

float resource_requirer::get_completed_frac()
{
    float resource_required = res_required.sum_absolute();
    float work_put_in = res_added.sum_absolute();

    if(resource_required < 0.00001f)
        return 1.f;

    return work_put_in / resource_required;
}

bool resource_requirer::is_completed()
{
    return get_completed_frac() >= 0.999999f;
}

float resource_requirer::get_resource_amount_required_to_complete_fraction(float frac)
{
    if(res_required.sum_absolute() < 0.00001f)
        return 0.f;

    float abs_val = res_required.sum_absolute();

    return frac * abs_val;
}

///maybe we want a collider that will return a collision and an optional movement vector
///then we can use this for cars as well
///we're no longer using the vector collision system
///this means that things going too fast can skip throuhg the wall
///however it has the capacity to be upgraded to handle that, so no major concern
vec2f moveable::tick(state& s, vec2f position, vec2f dir, float dist)
{
    if(dist < 0.00001f)
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

vec2f keyboard_controller::tick()
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

    return dir.norm();
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

///???
wall_segment_segment::wall_segment_segment(vec2f _start, vec2f _finish, float required_work)
{
    start = _start;
    finish = _finish;

    i1.set_position((finish - start).rot(M_PI/2.f).norm() * game::wall_segment_segment_interact_distance + (start + finish)/2.f), ///temp
    i2.set_position((finish - _start).rot(-M_PI/2.f).norm() * game::wall_segment_segment_interact_distance + (start + finish)/2.f), ///temp)

    i1.set_radius(game::wall_segment_segment_interact_rad);
    i2.set_radius(game::wall_segment_segment_interact_rad);

    construct.set_work_to_complete(required_work);

    ///this also needs to be pulled out
    res_require.set_resource_requried(air::IRON, game::wall_segment_segment_iron_needed);
}

void wall_segment_segment::tick(state& s, float dt)
{
    vec4f not_completed_col = game::wall_segment_segment_not_completed_col;

    not_completed_col = not_completed_col / 2.f + (not_completed_col / 2.f) * construct.get_completed_frac();

    vec4f completed_col = game::wall_segment_segment_completed_col;

    vec4f display_col;

    if(construct.is_constructed())
    {
        display_col = completed_col;
    }
    else
    {
        display_col = not_completed_col;
    }

    rect.tick(s, start, finish, game::wall_segment_thickness, display_col, game::wall_segment_outline_thickness);

    i1.tick(s);
    i2.tick(s);

    const float work_speed = game::wall_segment_segment_work_speed;

    if(i1.player_has_interacted_continuous(s) || i2.player_has_interacted_continuous(s))
    {
        player* play = s.current_player;

        float work_to_resource_frac = (work_speed * dt) / construct.max_work;

        float max_resource_usage_this_tick = res_require.get_resource_amount_required_to_complete_fraction(work_to_resource_frac);

        vecrf resources_available = play->player_resource_network.network_resources;


        ///test adding resources first, find out if we've exceeded our max amount of
        ///resources added per frame, then do
        resource_requirer test_resources = res_require;
        vecrf extra = test_resources.add(resources_available);

        vecrf diff = resources_available - extra;

        vecrf new_diff = diff;

        if(diff.sum_absolute() > max_resource_usage_this_tick && diff.sum_absolute() > 0.00001f)
        {
            new_diff = diff / diff.sum_absolute();
            new_diff = new_diff * max_resource_usage_this_tick;
        }

        /*printf("Used %f\n", new_diff.sum_absolute());
        printf("%f\n", diff.sum_absolute());
        printf("%f\n", max_resource_usage_this_tick);
        printf("%f\n", resources_available.v[air::FABRIC]);*/

        res_require.add(new_diff);
        play->player_resource_network.take(new_diff);

        float amount_of_work_by_material = (work_speed * dt) * new_diff.sum_absolute() / max_resource_usage_this_tick;

        construct.apply_work(amount_of_work_by_material);
    }
}

wall_segment::wall_segment(vec2f _start, vec2f _finish, float work_per_segment) : block(_start, _finish)
{
    start = _start;
    finish = _finish;

    work = work_per_segment;

    generate_sub_segments(work_per_segment);
}

void wall_segment::generate_sub_segments(float work_per_segment)
{
    sub_segments.clear();

    vec2f dir = (finish - start).norm();
    float length = (finish - start).length();

    int num = length/game::wall_segment_segment_length;
    dir = dir * game::wall_segment_segment_length;

    vec2f last_start = start;

    int n = 0;

    for(vec2f pos = start + dir; n <= num; pos = pos + dir, n++)
    {
        vec2f diff = pos - start;

        float travelled = diff.length();

        if(travelled > length)
        {
            pos = start + dir.norm() * length;
        }

        if((pos - last_start).length() < 0.1f)
            continue;

        sub_segments.push_back(wall_segment_segment(last_start, pos, work_per_segment));

        last_start = pos;
    }
}

void wall_segment::tick(state& s, float dt)
{
    bool all_complete = true;

    for(int i=0; i<(int)sub_segments.size(); i++)
    {
        sub_segments[i].tick(s, dt);

        all_complete = all_complete && sub_segments[i].construct.is_constructed();
    }

    if(all_complete)
    {
        sub_segments.clear();

        rect.tick(s, start, finish, game::wall_segment_thickness, game::wall_segment_colour, game::wall_segment_outline_thickness);

        block.tick(s);
    }
}

void wall_segment::destroy(state& s)
{
    block.destroy_remote(s);
}

/*std::vector<wall_segment> wall_splitter::split(state& s, float frac, wall_segment& seg)
{
    vec2f new_middle = mix(seg.start, seg.finish, frac);

    wall_segment s1(seg.start, new_middle);
    wall_segment s2(new_middle, seg.finish);

    if(seg.sub_segments.size() == 0)
    {
        s1.sub_segments.clear();
        s2.sub_segments.clear();
    }

    return {s1, s2};
}*/

///we need to deal with subsegments
/*wall_segment wall_segment::split_at_fraction(state& s, float frac)
{
    vec2f new_middle = mix(start, finish, frac);

    destroy(s);
    block.modify_bounds(start, new_middle);

    wall_segment new_segment(new_middle, finish);

    finish = new_middle;

    if(sub_segments.size() != 0)
        generate_sub_segments();
    else
        new_segment.sub_segments.clear();

    return new_segment;
}*/

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

area_interacter::area_interacter(vec2f _pos, float _radius) : area_interacter()
{
    pos = _pos;
    radius = _radius;
}

area_interacter::area_interacter()
{
    just_interacted = false;

    pos = 0.f;
    radius = 1.f;
}

void area_interacter::set_position(vec2f _pos)
{
    pos = _pos;
}

void area_interacter::set_radius(float _rad)
{
    radius = _rad;
}

///ehh ignore in config for the moment
void area_interacter::tick(state& s, bool gradient_centre)
{
    float outline = 0.5f;

    circle.tick(s, pos, radius - outline, (vec4f){220, 200, 200, 100}, outline);

    if(gradient_centre)
        circle.tick(s, pos, radius/2.f - outline, (vec4f){240, 240, 240, 100}, outline);
}

///this is wildly inefficient
std::vector<entity*> area_interacter::get_entities_within(state& s)
{
    std::vector<entity*> ret;

    for(auto& i : *s.entities)
    {
        vec2f position = i->position;

        vec2f rel = position - pos;

        float dist = rel.length();

        if(dist < radius)
        {
            ret.push_back(i);
        }
    }

    return ret;
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

bool area_interacter::player_has_interacted_continuous(state& s)
{
    if(!player_inside(s))
        return false;

    ///need to pull this logic out into a separate component
    sf::Keyboard key;

    if(key.isKeyPressed(sf::Keyboard::E))
    {
        return true;
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
        entity* ent = fetch_next_entity(fetch, s);

        if(ent)
            entities.push_back(ent);
    }

    return entities;
}

entity* saver::fetch_next_entity(byte_fetch& fetch, state& s)
{
    entity_t type = fetch.get<entity_t>();

    entity* ent = nullptr;

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

    else if(type == entity_type::REPAIR_ENTITY)
        ent = new repair_entity(fetch);

    else if(type == entity_type::ENVIRONMENT_BALANCER)
        ent = new environment_balancer(fetch);

    else if(type == entity_type::SUIT_ENTITY)
        ent = new suit_entity(fetch);

    else if(type == entity_type::RESOURCE_PACKET)
        ent = new resource_packet(fetch);

    else if(type == entity_type::RESOURCE_FILLER)
        ent = new resource_filler(fetch);

    else if(type == entity_type::RESOURCE_NETWORK_ENTITY)
        ent = new resource_network_entity(fetch);

    else if(type == entity_type::MINING_DRILL)
        ent = new mining_drill(fetch);

    return ent;
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

    //auto air_fracs = air_parts / air_parts.sum();
    //float air_pressure = air_parts.sum();
    ///we want to skip temperature here


    float air_pressure = 0.f;

    for(int i=0; i<air::COUNT; i++)
    {
        //if(i == air::TEMPERATURE)
        //    continue;

        air_pressure += air_parts.v[i];
    }

    auto air_fracs = air_parts / air_pressure;


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
        float val = air_fracs.v[i];

        ///alleviates the case of -0
        if(val < 0)
            val = 0;

        //if(i == air::TEMPERATURE)
        //    val = air_parts.v[i] / 100.f;

        display = display + air::names[i] + ": " + std::to_string(val * 100.f) + "%" + "\n";
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

void resource_displayer::set_element_to_display(resource_t type, bool val)
{
    should_display[type] = val;
}

void resource_displayer::tick(state& s, vec2f display_pos, const vecrf& resources, int size, bool absolute)
{
    bool display_all = should_display.size() == 0;

    auto resource_parts = resources;

    std::string display;

    for(int i=0; i<resource::RES_COUNT; i++)
    {
        if(resource_parts.v[i] <= 0.f)
            resource_parts.v[i] = 0.f;

        if(display_all || should_display[(resource_t)i])
            display = display + resource::names[i] + ": " + std::to_string(resource_parts.v[i]) + "\n";
    }

    if(!absolute)
        txt.render(s, display, display_pos, size, text_options::NONE);
    else
        txt.render(s, display, display_pos, size, text_options::ABS);
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

    float current_amount = local_environment.sum_absolute();

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
    float my_air = local_environment.sum_absolute();

    if(amount >= my_air)
        amount = my_air;

    if(my_air <= 0.00001f)
        return;

    //auto frac = local_environment / local_environment.sum();

    //auto to_emit = frac * amount;

    auto to_emit = (amount * local_environment) / local_environment.sum();

    //printf("Emitting %f\n", to_emit.sum());

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

/*byte_vector air_environment::get_save_data()
{
    byte_vector vec;
    vec.push_back<vecrf>(local_environment);

    return vec;
}*/

conditional_environment_modifier::conditional_environment_modifier()
{
    parent = nullptr;
}

float conditional_environment_modifier::get_pressure()
{
    return my_environment.local_environment.sum_absolute();
}

float conditional_environment_modifier::get_parent_pressure(state& s, vec2f pos)
{
    if(parent != nullptr)
        return parent->my_environment.local_environment.sum_absolute();

    return s.air_process->get(pos.v[0], pos.v[1]).sum_absolute();
}

///my goodness, this actually works *and* its not horrible!!
void conditional_environment_modifier::absorb_all(state& s, vec2f pos, float amount)
{
    if(parent == nullptr)
        my_environment.absorb_all(s, pos, amount, max_air);
    else
    {
        vecrf& parent_storage = parent->my_environment.local_environment;

        ///parent environment is empty, cannot absorb
        if(parent_storage.sum_absolute() < 0.00001f)
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
        //printf("Emitting %f %f %f\n", pos.v[0], pos.v[1], amount);

        my_environment.emit_all(s, pos, amount);
    }
    else
    {
        auto backup_parent = parent->parent;

        parent->parent = this;

        parent->absorb_all(s, pos, amount);

        parent->parent = backup_parent;
    }
}

vecrf conditional_environment_modifier::take(float amount)
{
    float amount_in_storage = my_environment.local_environment.sum();

    if(amount_in_storage < 0.00001f)
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

vecrf conditional_environment_modifier::take(vecrf amount)
{
    auto old = my_environment.local_environment;

    my_environment.local_environment = max(my_environment.local_environment - amount, 0.f);

    return old - my_environment.local_environment;
}

vecrf conditional_environment_modifier::add(vecrf amount)
{
    float total = amount.sum() + my_environment.local_environment.sum();

    if(total < 0.00001f || amount.sum() < 0.00001f)
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

vecrf conditional_environment_modifier::get()
{
    return my_environment.local_environment;
}

vecrf conditional_environment_modifier::get_parent(state& s, vec2f pos)
{
    if(parent != nullptr)
        return parent->get();

    return air_to_resource(s.air_process->get(pos.v[0], pos.v[1]));
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

body_model::body_model()
{
    pa_o2 = game::body_model_normal_o2_pa;
    pa_co2 = game::body_model_normal_co2_pa;
}

float body_model::get_o2_fraction_total()
{
    return pa_o2 / game::body_model_normal_o2_pa;
}

float body_model::get_o2_fraction_normal()
{
    return (pa_o2 - game::body_model_too_low_o2_pa) / (game::body_model_normal_o2_pa - game::body_model_too_low_o2_pa);
}

///hot damn, I checked this and its actually fucking correct
///what the actual christ, thats amazing
///http://medical-dictionary.thefreedictionary.com/oxygen+consumption
float body_model::get_gas_blood_volume_amount_atmospheric_ps_litres(float higher_pa, float lower_pa, float outside_atmospheric_pressure)
{
    //float pressure = pa_o2;
    //float volume = game::body_model_blood_volume_litres * 0.001f; ///to m3
    //float volume_litres = game::body_model_blood_volume_litres;
    //float moles = pressure * volume / (game::gas_R * game::body_T);

    //float o2_used_pa = game::body_model_normal_o2_pa - game::body_model_return_o2_pa;

    float gas_used_pa = higher_pa - lower_pa;

    ///moles per second usedish (moles / 0.001f)
    ///I think this is oxygen usage per second in molesish?
    ///blood_flow_litres_ps is total *volume*, but oxygen partial pressure assumes its whole volume
    ///so i think its valid to sub it in
    ///either way, ttod = 2 minutes which is good enough for me, i can game adjust that pretty easily
    ///and it seems logical
    float bps_moles_s = gas_used_pa * game::body_model_blood_flow_litres_ps / (game::gas_R * game::body_T);
    //float my_moles_s = pressure * game::body_model_blood_flow_litres_ps / (game::gas_R * game::body_T);

    //float body_moles_total = pa_o2 * game::body_model_blood_volume_litres / (game::gas_R * game::body_T);

    //float time_till_oxygen_depletion = body_moles_total / bps_moles_s;

    //printf("%f\n", time_till_oxygen_depletion);


    ///what we want is o2 used per second in litres

    ///eh fuck it, just assume the outside is at body temperature, its in the lungs
    ///and I can't be arsed to deal with gas expansion/contraction between a regulated 21 and 37 degrees
    ///...or can i?
    ///quite easy to adjust to a non linear o2_used_pa as well
    return game::gas_R * game::body_T * bps_moles_s / (outside_atmospheric_pressure);
}

float body_model::get_o2_blood_volume_used_atmospheric_ps_litres(float outside_pressure)
{
    return get_gas_blood_volume_amount_atmospheric_ps_litres(game::body_model_normal_o2_pa, game::body_model_return_o2_pa, outside_pressure);
}

float body_model::get_co2_blood_volume_used_atmospheric_ps_litres(float outside_pressure)
{
    return get_gas_blood_volume_amount_atmospheric_ps_litres(game::body_model_return_co2_pa, game::body_model_normal_co2_pa, outside_pressure);
}

/*void body_model::tick(float dt, float lung_pa_o2, float lung_pa_co2, float lung_volume, float lung_pa)
{
    float normal_ratio_mod = game::body_model_normal_o2_pa / (0.16f * air::atmospheric_pressure_pa);

    ///if > 0
    float pressure_differential = lung_pa_o2 * normal_ratio_mod - pa_o2;

    float change_this_tick = pressure_differential * dt;

    pa_o2 += change_this_tick;
}*/

#if 1
void body_model::tick(float dt, float lung_pa_o2, float lung_pa_co2, float lung_volume, float lung_pa)
{
    ///In reality, the diffusion rate is a function of lung_pa_o2 - blood_pa_o2
    ///however, we know that the body uses get_gas_blood_volume_amount_atmospheric litres per second
    ///so we can simply remove that from lung volume
    ///however.... really need to incorporate the above pressure problems
    ///so that

    ///we could literally just do lung_pa_o2 - blood_pa_o2 as the pressure available
    ///but because this is not accurate... it might end up fucked

    ///for the moment, we won't model o2 forcing itself into the bloodstream
    ///or boiling out of the blood into the lungs
    ///but its a very real possibility

    //dt = 1.f;

    float o2_pa_available = lung_pa_o2 - pa_o2;

    //if(o2_pa_available < 0)
    //    o2_pa_available = 0;

    if(o2_pa_available < -game::body_model_normal_o2_pa)
        o2_pa_available = -game::body_model_normal_o2_pa;

    float neg_o2_amount = o2_pa_available < 0.f ? fabs(o2_pa_available) : 0.f;

    //printf("available pao2 %f\n", o2_pa_available);
    //printf("lung pao2 %f\n", lung_pa_o2);
    //printf("blood pao2 %f\n", pa_o2);
    //printf("Lung pa %f\n", lung_pa);

    ///?
    float o2_volume_available = 0.f;

    if(lung_pa > 0.0001f)
        o2_volume_available = (o2_pa_available / lung_pa) * lung_volume;

    float o2_volume_wanting_to_use = 0.f;

    if(lung_pa > 0.0001f)
        o2_volume_wanting_to_use = get_o2_blood_volume_used_atmospheric_ps_litres(lung_pa);// * dt;

    //printf("wanting volume %f\n", o2_volume_wanting_to_use);
    //printf("available volume %f\n", o2_volume_available);


    ///volume
    float remaining = o2_volume_available - o2_volume_wanting_to_use;

    printf("lpo %f\n", lung_pa_o2);
    printf("pao2 %f\n", pa_o2);


    float amount_missing = 0.f;

    if(remaining < 0)
    {
        amount_missing = fabs(remaining);
        remaining = 0.f;
    }
    else
    {
        float upper = pa_o2;

        if(lung_pa_o2 > upper * 3.f)
        {
            amount_missing = -dt * 2.f;//- ((lung_pa_o2 - upper)) * dt / 12.f;
        }
    }

    ///amount of pa we want to use
    float o2_pa_diff = (game::body_model_normal_o2_pa - game::body_model_return_o2_pa);

    float frac_missing = 0.f;

    if(o2_volume_wanting_to_use > 0.00001f)
        frac_missing = amount_missing / o2_volume_wanting_to_use;
    else
        frac_missing = 1.f;

    float pa_missing = frac_missing * o2_pa_diff;

    //pa_missing = clamp(pa_missing, 0.f, o2_pa_diff);

    if(pa_missing > game::body_model_normal_o2_pa)
        pa_missing = game::body_model_normal_o2_pa;

    ///we wanna like, smooth this i think just by doing a moving averaage or something

    ///in seconds
    float blood_gas_inertia_factor = game::body_model_blood_volume_litres / game::body_model_blood_flow_litres_ps;

    pa_o2 = ((game::body_model_normal_o2_pa - pa_missing)*dt + pa_o2 * blood_gas_inertia_factor) / (blood_gas_inertia_factor+dt);

    ///extreme decompression
    if(lung_pa_o2 < pa_o2/game::body_model_environment_o2_less_than_frac_blood_o2)
    {
        pa_o2 = (lung_pa_o2*dt/game::body_model_o2_boil_time_s + pa_o2) / (1 + dt/game::body_model_o2_boil_time_s);
    }

    if(pa_o2 < 0)
        pa_o2 = 0;
}
#endif

breather::breather()
{
    ///temp hack
    current_time = -M_PI/2.f;

    is_holding_breath = false;
}

void breather::set_holding_breath_enabled(bool state)
{
    is_holding_breath = state;
}

///i think this assumes 1x1 square tiles
///need to swap to a proper breathing model
///FIXMEEE!!!
///volume of spacesuit without human is about (exactly) 5 foot^3
///volume of a liquified human being is about 2.344 foot^3
///volume of lungs is 0.2 ft^3
///tidal volume is 0.5l
///total volume is 0.6l
void breather::tick(state& s, vec2f position, float dt)
{
    ///lets put this into a breathing manager afterwards
    ///inhalation -> exhalation cycle
    float breaths_per_minute = 14;

    float breaths_per_second = breaths_per_minute / 60.f;

    ///4.3ish
    float breath_cycle_length_seconds = 1.f / breaths_per_second;

    ///litres (?)
    ///we can't actually do this as 6... this is too much
    ///maybe it should be m3
    const float lung_volume = game::breather_lung_air_volume;
    const float base_lung_air = lung_volume * game::breather_lung_frac_in_reserve;

    lungs.set_max_air(lung_volume + base_lung_air);


    float o2_to_co2_rate = breaths_per_second * dt * 0.2f;

    display.tick(s, (vec2f){20.f, 20.f}, resource_to_air(lungs.my_environment.local_environment), true);

    lungs.my_environment.convert_percentage(o2_to_co2_rate * lung_volume, 0.05f / 0.20f, air::OXYGEN, air::C02);

    float pressure = lungs.get_parent_pressure(s, position);
    float lvolume = lungs.get_pressure();

    ///am am bad at this
    if(lvolume <= 0.0001f)
        lvolume = 100000;

    float o2_frac = air::atmospheric_pressure_pa * lungs.my_environment.local_environment.v[air::OXYGEN] * pressure / lvolume;
    float co2_frac = air::atmospheric_pressure_pa * lungs.my_environment.local_environment.v[air::C02] * pressure / lvolume;

    ///6 is real lung volume in litres
    ///oh dear. Lungs.get_pressure() isn't pressure, its volume
    ///pressure is... environmental pressure?
    body.tick(dt, o2_frac, co2_frac, 6, air::atmospheric_pressure_pa * pressure);

    text txt;
    txt.render(s, "pa_o2 " + std::to_string(body.pa_o2 * game::pa_to_mmhg), (vec2f){20, 200}, 16, text_options::ABS);

    ///no need to do the rest of anything
    if(is_holding_breath)
        return;

    ///integral of sinx between 0 and PI is 2

    ///we need to integrate between time now, and next time, and inhale that amount of air

    float next_time = current_time + dt;

    if(next_time > breath_cycle_length_seconds)
        next_time -= breath_cycle_length_seconds;

    float c_frac = current_time / breath_cycle_length_seconds;
    float n_frac = next_time / breath_cycle_length_seconds;

    float s_c = c_frac * M_PI * 2;
    float s_n = n_frac * M_PI * 2;

    float current_sin = sin(s_c);
    float next_sin = sin(s_n);

    float air_change = lung_volume * (next_sin - current_sin)/2.f;

    ///need to make it much harder to inhale if the air is less than a certain amount
    ///or maybe we simply just need to buff lung volume, and air volume in suit
    ///eg we could standardise on gas volume
    if(air_change > 0)
    {
        lungs.absorb_all(s, position, air_change);
    }
    else
    {
        lungs.emit_all(s, position, -air_change);
    }

    if(lungs.get_pressure() < base_lung_air)
    {
        float diff = base_lung_air - lungs.get_pressure();

        lungs.absorb_all(s, position, diff);
    }


    /*float volume_to_exchange = -cos(s_n) + -cos(s_c);*/

    /*float current_air_volume = sin(s_c);
    float next_air_volume = sin(s_n);*/

    ///hmm no. Sinx is how much air is IN the lungs currently, not dvdt
    ///no thats also a lie, it is dvdt


    current_time += dt;

    if(current_time > breath_cycle_length_seconds)
        current_time -= breath_cycle_length_seconds;

    /*float ldt = dt * breaths_per_second;

    ///use this for some sort of hardcore realism mode where it takes 2 real days time.
    ///or maybe we can just accelerate time?
    //const float volume_per_breath_litres = 6; //real
    const float volume_per_breath_litres = 0.01;

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

    lungs.emit_all(s, position, 2.f);*/
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
    environment_absorb_rate = 0.f;
    environment_emit_rate = 0.f;
    pos = (vec2f){0.f, 0.f};
}

void resource_converter::set_air_absorb_rate(float _rate)
{
    environment_absorb_rate = _rate;
}

void resource_converter::set_air_emit_rate(float _rate)
{
    environment_emit_rate = _rate;
}

void resource_converter::set_air_transfer_rate(float _rate)
{
    set_air_absorb_rate(_rate);
    set_air_emit_rate(_rate);
}


void resource_converter::set_max_storage(const std::vector<std::pair<resource_t, float>>& lv)
{
    for(auto& i : lv)
    {
        max_storage.v[i.first] = i.second;
    }
}

void resource_converter::set_max_storage_vec(const vecrf& vec)
{
    max_storage = vec;
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

void resource_converter::set_usage(const vecrf& vec)
{
    conversion_usage_ratio = vec;
}

void resource_converter::set_output(const vecrf& vec)
{
    conversion_output_ratio = vec;
}

void resource_converter::add(const std::vector<std::pair<resource_t, float>>& lv)
{
    for(auto& i : lv)
    {
        local_storage.v[i.first] += i.second;
    }

    vecrf minimum;
    minimum = 0.f;

    local_storage = clamp(local_storage, minimum, max_storage);
}

vecrf resource_converter::take(const std::vector<std::pair<resource_t, float>>& lv)
{
    auto old = local_storage;

    for(auto& i : lv)
    {
        local_storage.v[i.first] -= i.second;
    }

    vecrf minimum;
    minimum = 0.f;

    local_storage = clamp(local_storage, minimum, max_storage);

    return old - local_storage;
}


vecrf resource_converter::add(const vecrf& res)
{
    auto prev = local_storage;

    local_storage = local_storage + res;

    local_storage = max(local_storage, 0.f);
    local_storage = min(local_storage, max_storage);

    //auto diff = local_storage - prev;

    return prev + res - local_storage;
}

vecrf resource_converter::take(const vecrf& res)
{
    auto prev = local_storage;

    local_storage = local_storage - res;

    local_storage = max(local_storage, 0.f);
    local_storage = min(local_storage, max_storage);

    auto diff = prev - local_storage;

    return diff;
}

///we want to deplete from local resources first, then escalate to global resources
///this is so that we can absorb a unit of gas from the environment, process it, then re-emit it afterwards
void convert_amount(float amount, vecrf& global_storage, vecrf& global_max, vecrf& local_environment, float dt, float efficiency, const vecrf& conversion_usage_ratio, const vecrf& conversion_output_ratio)
{
    if(amount <= 0.00001f)
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
    if(environment_absorb_rate > 0.f)
    {
        environment_absorption.absorb_all(s, pos, environment_absorb_rate * dt, environment_absorb_rate);

        //printf("%f\n", environmental_absorption_rate * dt);
    }
}

void resource_converter::emit_all(state& s, float dt)
{
    if(environment_emit_rate > 0.f)
    {
        environment_absorption.emit_all(s, pos, environment_emit_rate * dt);
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
    max_network_resources = 0.f;
    ///so that other networks can do my stuff for me
    ///and we won't double/ntuple tick
    processed = false;
    touched = false;
}

void resource_network::add_unique(resource_converter* conv)
{
    for(auto& i : converters)
    {
        if(i == conv)
            return;
    }

    add(conv);
}

void resource_network::add_net(resource_network* net)
{
    /*for(auto& i : connected_networks)
        if(i == net)
            return;*/

    auto nets = get_all_connected();

    for(auto& i : nets)
    {
        if(i == net)
            return;
    }

    if(net == this)
        return;

    connected_networks.push_back(net);
    net->connected_networks.push_back(this);

    network_resources = net->network_resources + network_resources;
    max_network_resources = net->max_network_resources + max_network_resources;

    distribute_fractionally_globally(network_resources);
}

void resource_network::add(resource_converter* conv)
{
    network_resources = network_resources + conv->local_storage;
    max_network_resources = max_network_resources + conv->max_storage;

    distribute_fractionally_globally(network_resources);

    converters.push_back(conv);
}

void resource_network::rem(resource_converter* conv)
{
    network_resources = network_resources - conv->local_storage;
    max_network_resources = network_resources - conv->max_storage;

    for(int i=0; i<(int)converters.size(); i++)
    {
        if(converters[i] == conv)
        {
            converters.erase(converters.begin() + i);
            i--;
        }
    }

    distribute_fractionally_globally(network_resources);
}

void resource_network::clear()
{
    network_resources = 0;
    converters.clear();
}

vecrf resource_network::add(const vecrf& res)
{
    auto old = network_resources;

    network_resources = min(network_resources + res, max_network_resources);

    auto diff = network_resources - old;

    distribute_fractionally_globally(network_resources);

    return res - diff;
}

vecrf resource_network::take(const vecrf& res)
{
    auto old = network_resources;

    network_resources = max(network_resources - res, 0.f);

    auto diff = old - network_resources;

    distribute_fractionally_globally(network_resources);

    return diff;
}

vecrf resource_network::get_local_max()
{
    vecrf local_max = 0.f;

    for(auto& i : converters)
    {
        local_max = local_max + i->max_storage;
    }

    return local_max;
}

vecrf resource_network::get_local_available()
{
    vecrf local_av = 0.f;

    for(auto& i : converters)
    {
        local_av = local_av + i->local_storage;
    }

    return local_av;
}

std::vector<resource_network*> resource_network::get_all_connected()
{
    std::vector<resource_network*> encountered = {this};
    std::vector<resource_network*> to_process = connected_networks;

    this->touched = true;

    while(to_process.size() != 0)
    {
        auto net = to_process.back();
        encountered.push_back(net);
        to_process.pop_back();

        for(auto& i : net->connected_networks)
        {
            if(!i->touched)
            {
                to_process.push_back(i);
                i->touched = true;
            }
        }
    }

    for(auto& i : encountered)
    {
        i->touched = false;
    }

    return encountered;
}

std::set<resource_converter*> resource_network::get_unique_converters()
{
    auto net = get_all_connected();

    std::set<resource_converter*> res;

    for(auto& i : net)
    {
        for(auto& j : i->converters)
        {
            res.insert(j);
        }
    }

    return res;
}

vecrf resource_network::get_global_max()
{
    /*auto nets = get_all_connected();

    vecrf global = 0.f;

    for(auto& i : nets)
    {
        global = global + i->get_local_max();
    }

    return global;*/

    auto convs = get_unique_converters();

    vecrf global = 0.f;

    for(resource_converter* i : convs)
    {
        global = global + i->max_storage;
    }

    return global;
}

vecrf resource_network::get_global_available()
{
    /*auto nets = get_all_connected();

    vecrf global = 0.f;

    for(auto& i : nets)
    {
        global = global + i->get_local_available();
    }

    return global;*/

    auto convs = get_unique_converters();

    vecrf global = 0.f;

    for(resource_converter* i : convs)
    {
        global = global + i->local_storage;
    }

    return global;
}

void resource_network::zero_local_containers()
{
    for(auto& i : converters)
    {
        i->local_storage = 0.f;
    }
}

void resource_network::zero_global_containers()
{
    auto net = get_all_connected();

    for(auto& i : net)
        i->zero_local_containers();
}

void resource_network::distribute_fractionally_globally(const vecrf& amount)
{
    auto convs = get_unique_converters();

    vecrf global_max = get_global_max();

    float total_storage = global_max.sum_absolute();

    if(total_storage < 0.00001f)
        return;

    for(auto& c : convs)
    {
        vecrf storage = c->max_storage;

        vecrf frac_to_store;

        for(int i=0; i<resource::RES_COUNT; i++)
        {
            if(global_max.v[i] < 0.00001f)
                frac_to_store.v[i] = 0;
            else
            {
                ///give me storage relative to the overall storage
                frac_to_store.v[i] = storage.v[i] / global_max.v[i];
            }
        }

        c->local_storage = frac_to_store * amount;
    }

    auto nets = get_all_connected();

    for(auto& i : nets)
    {
        i->network_resources = amount;
        i->max_network_resources = global_max;
    }

    /*auto net = get_all_connected();

    vecrf global_storage = get_global_max();

    float total_storage = global_storage.sum_absolute();

    if(total_storage < 0.00001f)
        return;

    for(resource_network*& i : net)
    {
        vecrf storage = i->get_local_max();

        float frac = storage.sum_absolute() / total_storage;

        i->distribute_fractionally_locally(frac * amount);

        //i->add
    }*/
}

void resource_network::distribute_fractionally_locally(const vecrf& amount)
{
    for(auto& c : converters)
    {
        auto storage = c->max_storage;

        vecrf frac_to_store;

        for(int i=0; i<resource::RES_COUNT; i++)
        {
            if(max_network_resources.v[i] < 0.00001f)
                frac_to_store.v[i] = 0;
            else
            {
                ///give me storage relative to the overall storage
                frac_to_store.v[i] = storage.v[i] / max_network_resources.v[i];
            }
        }

        auto local_frac = amount * frac_to_store;

        c->local_storage = local_frac;
    }
}

void resource_network::distribute_lump_locally(const vecrf& amount)
{
    auto amount_to_allocate = amount;

    for(resource_converter*& c : converters)
    {
        c->local_storage = 0.f;
        vecrf leftover = c->add(amount_to_allocate);

        vecrf diff = amount_to_allocate - leftover;

        amount_to_allocate = amount_to_allocate - diff;

        amount_to_allocate = max(amount_to_allocate, 0.f);
    }
}

///no processing done here
///only resource distribution
///distribute equally? or proportionally?
///in the long term we'll want to set up container priorities
///we also need to be able to extract from the network
///proportional is easiest
///I'm going to have to scrap this and do it properly, aren't I
///gunna have to deal with deleted resource networks later somehow

///so at the moment, the network_resources is broken
///so when we add to the network, it needs to get stapeled onto ones network resources (fine), which will auto distribute
///when we take from the network, we need to lump that shit between the network any way we care too
///or maybe we dont, we can just distribute globally because it dont matter?
///or maybe it does

void resource_network::tick(state& s, float dt, bool lump)
{
    if(processed)
    {
        processed = false;
        return;
    }

    max_network_resources = get_global_max();

    ///this is better
    ///but means if resources are destroyed, we'll only lose them if the network cant store the extra capacity
    ///which is sort of not really what we want
    network_resources = min(network_resources, max_network_resources);

    zero_global_containers();

    auto conv = get_unique_converters();

    //printf("%i\n", conv.size());

    for(resource_converter* i : conv)
    {
        i->absorb_all(s, dt);
        //printf("%f\n", i->environment_absorption.local_environment.v[air::C02]);
        i->convert(network_resources, max_network_resources, dt);
        i->emit_all(s, dt);
    }

    auto nets = get_all_connected();

    for(auto& i : nets)
    {
        if(i != this)
            i->processed = true;
    }

    ///distribute resources proportionally
    ///if something is destroyed, we'll lose the
    ///proportional resources from the network
    if(!lump)
    {
        distribute_fractionally_globally(network_resources);
    }
    else
    {
        distribute_lump_locally(network_resources);
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

damageable::damageable()
{
    health = 1.f;
}

void damageable::damage_amount(float fraction)
{
    health -= fraction;

    health = std::max(health, 0.f);
}

float damageable::heal_amount(float fraction)
{
    health += fraction;

    float extra = health > 1.f ? health - 1.f : 0.f;

    health = clamp(health, 0.f, 1.f);

    return extra;
}

void damageable::reset()
{
    health = 1.f;
}

bool damageable::is_alive()
{
    return health > 0.f;
}

float damageable::get_health_frac()
{
    return std::max(health, 0.f);
}

float suit_part::get_leak_rate()
{
    float damage_start = 0.05f;

    float my_damage = 1.f - damage.get_health_frac();

    if(my_damage < damage_start)
        return 0.f;

    ///so eg 0.5 - 0.2f / 0.8 = 1.f
    float val = (my_damage - damage_start) / (1.f - damage_start);

    return val * game::health_to_leak_conversion;
}

suit_status_displayer::suit_status_displayer()
{
    //file.load("./res/02.png");
}

void suit_status_displayer::tick(state& s, suit& mysuit)
{
    /*int width = s.win->getSize().x;
    int height = s.win->getSize().y;

    float scale = 0.3f;

    int mywidth = file.tex.getSize().x * scale;
    int myheight = file.tex.getSize().y * scale;

    file.tick(s, (vec2f){mywidth/2.f, height - myheight/2.f}, scale, 0.f, false, true);*/

    text status;

    std::string to_display = "Suit Integrity:\n\n";

    for(int i=0; i<suit_parts::COUNT; i++)
    {
        suit_part part = mysuit.parts[(suit_t)i];

        to_display = to_display + suit_parts::names[i] + ": " + std::to_string(part.damage.get_health_frac() * 100.f) + "%";

        if(part.get_leak_rate() > 0.f)
        {
            to_display = to_display + " LEAKING!!";
        }

        to_display = to_display + "\n";
    }

    to_display = to_display + "\nTotal leak rate: " + std::to_string(mysuit.get_total_leak()) + "\n";

    status.render(s, to_display, (vec2f){50.f, s.win->getSize().y - 230.f}, 10, text_options::ABS);
}

suit::suit()
{
    for(int i=0; i<suit_parts::COUNT; i++)
    {
        parts[(suit_t)i] = suit_part();
    }

    balancer = new environment_balancer(suit_resource_network);
    balancer->environment.set_parent(&environment);
    balancer->set_environment_target(game::get_suit_ideal_environment());

    resource_storage.set_max_storage_vec(game::get_suit_resource_max_storage());

    resource_storage.local_storage = game::get_suit_init_storage();

    suit_resource_network.add(&resource_storage);

    ///absorption rate
    environment.set_max_air(10000.f);
    environment.my_environment.local_environment = game::get_suit_init_environment(); ///temp
    //environment.my_environment.local_environment.v[air::TEMPERATURE] = 294;
    ///hmm. This isn't really ideal modelling temperature as a gas

    ///temp, doing damage to suit
    //parts[suit_parts::LARM].damage.damage_amount(0.6f);
}

///this is actually very cool
void suit::tick(state& s, float dt, vec2f pos)
{
    display.tick(s, pos + (vec2f){-20, -10.f}, resource_to_air(environment.my_environment.local_environment));

    resource_storage.set_position(pos);
    balancer->set_position(pos);

    suit_resource_network.tick(s, dt);

    resource_displayer res_display;
    res_display.tick(s, (vec2f){400, 10.f}, suit_resource_network.network_resources, 16, true);

    //printf("%f\n", suit_resource_network.network_resources.v[air::OXYGEN]);

    balancer->process_environment(s, dt);

    ///per second
    float leak_rate = get_total_leak();

    //printf("%f\n", leak_rate * dt);

    //environment.set_parent(nullptr);
    //environment.emit_all(s, pos, leak_rate * dt);

    float my_pressure = environment.get_pressure();
    float parent_pressure = environment.get_parent_pressure(s, pos);

    //float avg_pressure = (my_pressure + parent_pressure) / 2.f;

    //float pressure_difference = avg_pressure - my_pressure;

    ///this is how fast EVERYONE is equalising
    float equalisation_constant = leak_rate * game::leak_to_pressure_normalisation_fraction;

    equalisation_constant = clamp(equalisation_constant, 0.f, 1.f);

    //float pressure_difference_to_them = avg_pressure - my_pressure;
    //float pressure_difference_to_me = avg_pressure - parent_pressure;

    ///to them
    float pressure_difference = parent_pressure - my_pressure;

    ///no equalisation needed, give up
    if(equalisation_constant <= 0.00000001f)
    {
        return;
    }

    ///if they're almost at the same amount, slow the gas exchange to passive diffusion levels
    ///this is basically the switch between advection mediated flow, and diffusion
    ///not really scientifically accurate, but what are you gunna do
    if(fabs(pressure_difference) <= 0.001f)
    {
        equalisation_constant /= 100.f;

        printf("Pressure throttling\n");
    }

    ///theirs higher than mine
    ///because this clause is all inclusive
    ///some pressure equalisation will always take place
    ///which is what we want, to force advection of gases
    if(pressure_difference > 0)
    {
        environment.absorb_all(s, pos, equalisation_constant);
    }
    else
    {
        environment.emit_all(s, pos, equalisation_constant);
    }

    ///so now what we actually wanna do is equalise dependent on leak rate and the pressure difference
}

float suit::repair(float amount)
{
    float remaining = amount;

    for(auto& i : parts)
    {
        remaining = i.second.damage.heal_amount(remaining);

        ///can't be less than 0, but guard against my own poor programming
        ///also floats are evil under -Ofast
        if(remaining <= 0.f)
            return 0.f;
    }

    return remaining;
}

float suit::get_total_leak()
{
    float accum = 0.f;

    for(auto& i : parts)
    {
        accum += i.second.get_leak_rate();
    }

    return accum;
}

float suit::get_total_damage()
{
    float accum = 0.f;

    for(auto& i : parts)
    {
        accum += 1.f - i.second.damage.get_health_frac();
    }

    return accum;
}

mass::mass()
{
    amount = 0.f;
}

void mass::set_mass(float _amount)
{
    amount = _amount;
}

///well, mass modifier really..
float mass::get_velocity_modifier()
{
    if(amount >= 0.001f)
    {
        return 1.f / amount;
    }
    else
    {
        return 0.f;
    }
}

momentum_handler::momentum_handler()
{
    velocity = 0.f;
}

void momentum_handler::set_mass(float _amount)
{
    mymass.set_mass(_amount);
}

vec2f momentum_handler::do_movement(state& s, vec2f position, vec2f dir, float dist, float dt, float slowdown_frac)
{
    dist = dist * dt;

    slowdown_frac = clamp(slowdown_frac * (dt / 0.01f), 0.f, slowdown_frac);

    slowdown_frac = clamp(slowdown_frac, 0.f, 1.f);

    vec2f to_move = velocity + (dir.norm() * dist * mymass.get_velocity_modifier()) * dt / 0.01f;

    moveable mov;
    vec2f new_pos = mov.tick(s, position, to_move.norm(), to_move.length());

    ///this needs to be based on dt and mass
    float slowdown_factor = slowdown_frac;

    vec2f new_vel = (new_pos - position) * slowdown_factor;

    velocity = new_vel + (new_vel - velocity);

    if(velocity.length() > dist)
    {
        velocity = velocity.norm() * dist;
    }

    //printf("%f\n", velocity.length());

    return new_pos;
}

///sometimes I wonder if I'm going much too far with this entity component system
///then I remember that this project is going so well because of the ridiculous overencapsulation of everything
///or perhaps I only feel like that because of how much useless code I've written
repair_component::repair_component()
{
    repair_remaining = 1.f;
}

float repair_component::deplete(float amount)
{
    float initial = repair_remaining;

    repair_remaining -= amount;

    repair_remaining = std::max(repair_remaining, 0.f);

    ///amount we could't use
    return initial - repair_remaining;
}

float repair_component::add(float amount)
{
    repair_remaining += amount;

    float extra = repair_remaining > 1.f ? repair_remaining - 1.f : 0.f;

    repair_remaining = std::min(repair_remaining, 1.f);

    return extra;
}


