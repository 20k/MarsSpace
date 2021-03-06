#include "air.hpp"
#include "components.h"
#include <fstream>

///I'm going to do this the inefficient way :[
///intially assume the surface of mars is a vacuum
///but later we'll want to model the actual crap

vec<air::COUNT, float> air_processor::martian_atmosphere = get_martian_atmosphere();

void air_processor::load(int _width, int _height)
{
    width = _width;
    height = _height;

    buf = new vec<N, float>[width*height];
    buf_out = new vec<N, float>[width*height];

    ///this seems idiotic, but it runs 2ms faster
    ///I'm not going to argue with that
    for(int i=0; i<width*height; i++)
    {
        buf[i] = martian_atmosphere;
        buf_out[i] = martian_atmosphere;
    }
}

void air_processor::add(int x, int y, float amount, air_t type)
{
    if(x < 0 || y < 0 || x >= width || y >= width)
        return;

    buf[y*width + x].v[type] += amount;
    buf[y*width + x].v[type] = std::max(buf[y*width + x].v[type], 0.f);
}

float air_processor::take(int x, int y, float amount, air_t type)
{
    if(x < 0 || y < 0 || x >= width || y >= width)
        return 0.f;

    float old_amount = buf[y*width + x].v[type];

    buf[y*width + x].v[type] -= amount;
    buf[y*width + x].v[type] = std::max(buf[y*width + x].v[type], 0.f);

    return old_amount - buf[y*width + x].v[type];
}

vec<air_processor::N, float> air_processor::take_volume(int x, int y, float amount)
{
    if(x < 0 || y < 0 || x >= width || y >= width || amount <= 0)
    {
        vec<N, float> ret;
        ret = 0.f;
        return ret;
    }

    float cur_amount = buf[y*width + x].sum();

    if(amount > cur_amount)
        amount = cur_amount;

    if(cur_amount < 0.0001f)
    {
        vec<N, float> ret;
        ret = 0.f;
        return ret;
    }

    auto fracs = buf[y*width + x] / cur_amount;

    buf[y*width + x] = buf[y*width + x] - fracs * amount;

    buf[y*width + x] = max(buf[y*width + x], 0.f);

    return fracs * amount;
}

void air_processor::add_volume(int x, int y, const vec<air_processor::N, float>& amount)
{
    if(x < 0 || y < 0 || x >= width || y >= width)
        return;

    auto vec = amount;

    vec = max(vec, 0.f);

    buf[y*width + x] = buf[y*width + x] + amount;
}

///it is wildly inefficient to do this per frame
///doors closing will currently destroy atmosphere
void air_processor::draw_lines(state& s)
{
    for(auto& b : s.blockers)
    {
        vec2f dir;
        int num;

        line_draw_helper(b->start, b->finish, dir, num);

        int n = 0;

        for(vec2f pos = b->start; n <= num; pos = pos + dir, n++)
        {
            if(pos.v[0] < 0 || pos.v[0] >= width || pos.v[1] < 0 || pos.v[1] >= height)
                continue;

            vec2f r_pos = round(pos);

            for(int i=0; i<N; i++)
                buf[(int)r_pos.v[1] * width + (int)r_pos.v[0]].v[i] = -1.f;
        }
    }
}

///this is going to have to be gpu ported
///luckily not too impossibru
void air_processor::tick(state& s, float dt)
{
    draw_lines(s);

    //for(int k=0; k<5; k++)
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            ///boundary. Ideally we want this to be martian gas, but for the moment
            ///its just a vacuum
            if(x == 0 || x == width-1 || y == 0 || y == height-1)
            {
                buf_out[y*width + x] = martian_atmosphere;
                continue;
            }

            buf_out[y*width + x] = 0.f;

            float is_blocked = buf[y*width + x].v[0];

            ///using this as a blocked flag
            if(is_blocked < 0)
                continue;

            vec<N, float> my_val = buf[y*width + x];

            vec<4, vec<N, float>> vals = {
                buf[y*width + x + 1],
                buf[y*width + x - 1],
                buf[(y+1)*width + x],
                buf[(y-1)*width + x]
            };

            ///I'm the last person to ever touch this pixel
            ///and therefore it is safe for me to reset it if its set to blocked
            for(int i=0; i<N; i++)
            {
                if(buf[(y-1)*width + x].v[i] < 0)
                {
                    buf_out[(y-1)*width + x].v[i] = 0;
                    buf[(y-1)*width + x].v[i] = 0;
                }
            }

            vec<N, float> total;
            vec<N, float> nums;

            total = 0.f;
            nums = 0.f;

            for(int i=0; i<4; i++)
            {
                for(int j=0; j<N; j++)
                {
                    if(vals.v[i].v[j] > 0.000001f)
                    {
                        total.v[j] += vals.v[i].v[j];
                        nums.v[j]++;
                    }
                }
            }

            float diffusion_constant = 0.1f;

            vec<N, float> fin = (diffusion_constant * my_val + total) / (nums + diffusion_constant);

            buf_out[y*width + x] = fin;
        }
    }

    auto backup = buf;
    buf = buf_out;
    buf_out = backup;
}

void air_processor::draw(state& s)
{
    sf::Image img;
    img.create(width, height);

    sf::RenderStates rs(sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One));

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            ///if we're exactly 0, make me the average of my neighbours instead
            vec<N, float> val = buf[y*width + x];
            val = clamp(val, 0.f, 1.f);

            val = val * 255.f;

            img.setPixel(x, y, sf::Color(val.v[air::NITROGEN], val.v[air::C02], val.v[air::OXYGEN], 128));
        }
    }

    sf::Texture tex;
    tex.loadFromImage(img);
    tex.setSmooth(true);

    sf::Sprite spr;
    spr.setPosition(-0.5, -0.5);
    spr.setTexture(tex);

    s.win->draw(spr, rs);
}

vec<air_processor::N, float> air_processor::get(float x, float y)
{
    x = round(x);
    y = round(y);

    if(x < 0 || y < 0 || x >= width || y >= height)
    {
        vec<N, float> ret;
        ret = 0;

        return ret;
    }

    return buf[(int)y*width + (int)x];
}

void air_processor::save_to_file(const std::string& fname)
{
    FILE* pFile = fopen(fname.c_str(), "wb");

    fwrite(buf, sizeof(vec<air::COUNT, float>) * width * height, 1, pFile);

    fclose(pFile);
}

void air_processor::load_from_file(const std::string& fname)
{
    char* ptr = (char*)buf;

    std::ifstream in(fname, std::ios::in | std::ios::binary);
    if (in)
    {
        in.seekg(0, std::ios::end);
        int size = in.tellg();
        in.seekg(0, std::ios::beg);
        in.read(ptr, size);
        in.close();
    }
}
