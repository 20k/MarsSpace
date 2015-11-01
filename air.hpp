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
        TOXIC,
        COUNT
    };
}

///I'm going to do this the inefficient way :[
///intially assume the surface of mars is a vacuum
///but later we'll want to model the actual crap
struct air_processor
{
    static constexpr int N = air::COUNT;
    vec<N, float>* buf;
    int width, height;

    void load(int _width, int _height)
    {
        width = _width;
        height = _height;

        buf = new vec<N, float>[width*height];

        ///this seems idiotic, but it runs 2ms faster
        ///I'm not going to argue with that
        for(int i=0; i<width*height; i++)
        {
            for(auto& k : buf[i].v)
                k = 0.00001f;
            //buf[i] = 0.00001f;
        }
    }

    void add(int x, int y, float amount)
    {
        if(x < 0 || y < 0 || x >= width || y >= width)
            return;

        for(int i=0; i<N; i++)
            buf[y*width + x].v[i] += amount;
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

                for(int i=0; i<N; i++)
                    buf[(int)r_pos.v[1] * width + (int)r_pos.v[0]].v[i] = -1.f;
            }
        }
    }

    void tick(state& s, float dt)
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
                    buf[y*width + x] = 0;
                    continue;
                }

                float my_val = buf[y*width + x].v[0];

                ///using this as a blocked flag
                if(my_val < 0)
                    continue;

                vec<4, vec<N, float>> vals = {
                    buf[y*width + x + 1],
                    buf[y*width + x - 1],
                    buf[(y+1)*width + x],
                    buf[(y-1)*width + x]
                };

                ///I'm the last person to ever touch this pixel
                ///and therefore it is safe for me to reset it if its set to blocked
                /*if(buf[(y-1)*width + x] < 0)
                {
                    buf[(y-1)*width + x] = 0;
                }*/

                for(int i=0; i<N; i++)
                {
                    if(buf[(y-1)*width + x].v[i] < 0)
                        buf[(y-1)*width + x].v[i] = 0;
                }

                //vals = max(vals, 0.f);

                //float total = vals.sum();

                vec<N, float> total;
                vec<N, float> nums;

                total = 0.f;
                nums = 0.f;

                for(int i=0; i<4; i++)
                {
                    /*if(vals.v[i] >= 0)
                    {
                        total += vals.v[i];
                        ++num;
                    }*/

                    for(int j=0; j<N; j++)
                    {
                        if(vals.v[i].v[j] >= 0)
                        {
                            total.v[j] += vals.v[i].v[j];
                            nums.v[j]++;
                        }
                    }
                }

                float diffusion_constant = 0.1f;

                vec<N, float> fin = (diffusion_constant * my_val + total) / (nums + diffusion_constant);

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
                vec<N, float> val = buf[y*width + x];
                val = clamp(val, 0.f, 1.f);

                val = val * 255.f;

                img.setPixel(x, y, sf::Color(val.v[0], val.v[0], val.v[1], 128));
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
