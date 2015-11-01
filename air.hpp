#ifndef AIR_HPP_INCLUDED
#define AIR_HPP_INCLUDED

#include "components.h"
#include <vec/vec.hpp>

namespace air
{
    ///we could include argon, but its basically the same as nitrogen
    enum air : uint8_t
    {
        HYDROGEN = 0,
        NITROGEN,
        OXYGEN,
        C02,
        WATER,
        TOXIC
    };
}

///I'm going to do this the inefficient way :[
///intially assume the surface of mars is a vacuum
///but later we'll want to model the actual crap
struct air_processor
{
    float* buf;
    int width, height;

    void load(int _width, int _height)
    {
        width = _width;
        height = _height;

        buf = new float[width*height]();

        ///this seems idiotic, but it runs 2ms faster
        ///I'm not going to argue with that
        for(int i=0; i<width*height; i++)
        {
            buf[i] = 0.00001f;
        }
    }

    void add(int x, int y, float amount)
    {
        if(x < 0 || y < 0 || x >= width || y >= width)
            return;

        buf[y*width + x] += amount;
    }

    ///it is wildly inefficient to do this per frame
    void draw_lines(state& s)
    {
        for(auto& b : s.blockers)
        {
            vec2f start = b->start;
            vec2f finish = b->finish;

            vec2f dir = (finish - start);
            float dist = dir.largest_elem();

            dir = dir / dist;

            int num = dist;
            int n = 0;

            for(vec2f pos = start; n <= num; pos = pos + dir, n++)
            {
                if(pos.v[0] < 0 || pos.v[0] >= width || pos.v[1] < 0 || pos.v[1] >= height)
                    continue;

                vec2f r_pos = round(pos);

                buf[(int)r_pos.v[1] * width + (int)r_pos.v[0]] = -1.f;
            }
        }
    }

    void tick(state& s, float dt)
    {
        draw_lines(s);

        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                ///boundary. Ideally we want this to be martian gas, but for the moment
                ///its just a vacuum
                if(x == 0 || x == width-1 || y == 0 || y == height-1)
                {
                    buf[y*width + x] = 0;
                    continue;
                }

                float my_val = buf[y*width + x];

                ///using this as a blocked flag
                if(my_val < 0)
                    continue;

                vec<4, float> vals = {
                    buf[y*width + x + 1],
                    buf[y*width + x - 1],
                    buf[(y+1)*width + x],
                    buf[(y-1)*width + x]
                };

                ///I'm the last person to ever touch this pixel
                ///and therefore it is safe for me to reset it if its set to blocked
                if(buf[(y-1)*width + x] < 0)
                {
                    buf[(y-1)*width + x] = 0;
                }

                //vals = max(vals, 0.f);

                //float total = vals.sum();

                float total = 0;
                int num = 0;

                for(int i=0; i<4; i++)
                {
                    if(vals.v[i] >= 0)
                    {
                        total += vals.v[i];
                        ++num;
                    }
                }

                float diffusion_constant = 0.1f;

                float fin = (diffusion_constant * my_val + total) / (num + diffusion_constant);

                buf[y*width + x] = fin;
            }
        }
    }

    void draw(state& s)
    {
        sf::Image img;
        img.create(width, height);

        sf::RenderStates rs(sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One));

        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                float val = buf[y*width + x];
                val = clamp(val, 0.f, 1.f);

                val = val * 255;

                img.setPixel(x, y, sf::Color(val, val, val, 128));
            }
        }

        sf::Texture tex;
        tex.loadFromImage(img);

        sf::Sprite spr;
        spr.setPosition(-0.5, -0.5);
        spr.setTexture(tex);

        s.win->draw(spr, rs);
    }
};

#endif // AIR_HPP_INCLUDED
