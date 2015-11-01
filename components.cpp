#include "components.h"
#include "entities.h"
#include <fstream>
#include <ostream>

uint32_t movement_blocker::gid = 0;

state::state(sf::RenderWindow* _win, sf::Texture& tex)
{
    win = _win;
    planet_tex = tex;

    current_player = nullptr;
}

void renderable_file::load(const std::string& name)
{
    img.loadFromFile(name.c_str());
    tex.loadFromImage(img);
    tex.setSmooth(true);
}

void renderable_file::tick(state& s, vec2f pos)
{
    sf::Sprite spr;
    spr.setTexture(tex);

    int width = tex.getSize().x;
    int height = tex.getSize().y;

    float xp = pos.v[0];
    float yp = pos.v[1];

    spr.setOrigin(width/2.f, height/2.f);
    spr.setPosition(xp, yp);
    spr.setScale(0.1f, 0.1f);

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

void renderable_circle::tick(state& s, vec2f pos, float rad)
{
    sf::CircleShape circle;
    circle.setOrigin(rad, rad);
    circle.setRadius(rad);

    circle.setPosition(pos.v[0], pos.v[1]);

    circle.setFillColor(sf::Color(220, 220, 220, 100));
    circle.setOutlineThickness(0.5f);
    circle.setOutlineColor(sf::Color(220, 220, 220, 50));

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
vec2f moveable::tick(state& s, vec2f position, vec2f dir, float dist)
{
    vec2f new_pos = position + dir.norm() * dist;

    for(auto& i : s.blockers)
    {
        ///degenerate
        if(i->start == i->finish)
            continue;

        bool s1 = is_left_side(i->start, i->finish, position);
        bool s2 = is_left_side(i->start, i->finish, new_pos);

        vec2f avg = (i->start + i->finish) / 2.f;

        ///padme?
        float rad = (i->finish - i->start).length() / 2.f;

        float dist_to_line_centre = (position - avg).length();


        vec2f to_wall = point2line_shortest(i->start, (i->finish - i->start), position);

        float dist_to_wall = to_wall.length();

        const float when_to_start_perp = 0.9f;
        const float pad = 1.f;

        if((s1 != s2 && dist_to_line_centre <= rad) || (dist_to_wall < when_to_start_perp && dist_to_line_centre <= rad))
        {
            ///so, we want to get the current vector
            ///from me to the wall
            ///and my movement vector dir
            ///and then cancel out the to the wall component

            ///degenerate case where we exactly intersect the wall line
            ///and we're right at the endge too
            ///if we end up teleporting through walls, this is prolly why
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

            vec2f perp = to_wall.rot(M_PI/2.f);

            perp = perp.norm();
            vec2f ndir = dir.norm();

            if(dot(perp, ndir) < 0)
                perp = -perp;

            float extra = std::max(pad - dist_to_wall, 0.f);

            return position + perp * dist - to_wall.norm() * extra;
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
    circle.tick(s, pos, radius);
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

        entities.push_back(ent);
    }

    return entities;
}
