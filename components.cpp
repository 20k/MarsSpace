#include "components.h"
#include "entities.h"

uint32_t movement_blocker::gid = 0;

state::state(sf::RenderWindow* _win)
{
    win = _win;

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

    float xp = pos.v[0] - width/2.f;
    float yp = pos.v[1] - height/2.f;

    spr.setPosition(xp, yp);

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
    circle.setOrigin(rad/2.f, rad/2.f);

    circle.setPosition(pos.v[0], pos.v[1]);

    circle.setFillColor(sf::Color(220, 220, 220));
    circle.setOutlineThickness(0.5f);
    circle.setOutlineColor(sf::Color(220, 220, 220, 128));

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
    rect.setOutlineColor(sf::Color(130,130,130, 200));

    s.win->draw(rect);
}

///maybe we want a collider that will return a collision and an optional movement vector
///then we can use this for cars as well
vec2f moveable::tick(state& s, vec2f position, vec2f dir, float dist)
{
    vec2f new_pos = position + dir.norm() * dist;

    for(auto& i : s.blockers)
    {
        bool s1 = is_left_side(i->start, i->finish, position);
        bool s2 = is_left_side(i->start, i->finish, new_pos);

        vec2f avg = (i->start + i->finish) / 2.f;

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

movement_blocker::movement_blocker(state& _s, vec2f _start, vec2f _finish)
{
    start = _start;
    finish = _finish;

    id = gid++;

    s = &_s;

    remote = std::make_shared<movement_blocker>(*this);

    s->blockers.push_back(remote);

    printf(":)\n");
}

///you know, i'm not sure this actually works
movement_blocker::~movement_blocker()
{
    if(s == nullptr)
        return;

    for(int i=0; i<(int)s->blockers.size(); i++)
    {
        ///I'm the last
        ///wait, if its a shared pointer it owns itself as wlel
        ///so this is 2
        ///but that means if im deconstructed and then reconstructed
        ///then broken
        ///so, rip in peace for the moment
        if(s->blockers[i].use_count() == 1)
        {
            printf(":(\n");

            s->blockers.erase(s->blockers.begin() + i);
        }
    }
}

wall_segment::wall_segment(state& s, vec2f _start, vec2f _finish) : block(s, _start, _finish)
{
    start = _start;
    finish = _finish;
}

void wall_segment::tick(state& s)
{
    rect.tick(s, start, finish, 1.f);
}

vec2f mouse_fetcher::get(state& s)
{
    sf::Mouse mouse;

    int x = mouse.getPosition(*s.win).x;
    int y = mouse.getPosition(*s.win).y;

    auto mouse_pos = s.win->mapPixelToCoords({x, y});

    return {mouse_pos.x, mouse_pos.y};
}

area_interacter::area_interacter(vec2f _pos, float _radius)
{
    pos = _pos;
    radius = _radius;
    just_interactd = false;
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

    if(dist < rad)
    {
        return true;
    }

    return false;
}

bool area_interacter::player_has_interacted(state& s)
{
    if(!player_inside(s))
        return false;

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
}
