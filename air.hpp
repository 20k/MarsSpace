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
        /*for(int i=0; i<width*height; i++)
        {
            //buf[i] = 0.0001f;
        }*/
    }

    void add(int x, int y, float amount)
    {
        if(x < 0 || y < 0 || x >= width || y >= width)
            return;

        buf[y*width + x] += amount;
    }

    void tick(float dt)
    {
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

                float v1, v2, v3, v4;
                v1 = buf[y*width + x + 1];
                v2 = buf[y*width + x - 1];
                v3 = buf[(y+1)*width + x];
                v4 = buf[(y-1)*width + x];

                float total = v1 + v2 + v3 + v4;

                float diffusion_constant = 5.f;

                float fin = (diffusion_constant * buf[y*width + x] + total) / (4.f + diffusion_constant);

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
        spr.setTexture(tex);

        s.win->draw(spr, rs);
    }
};

#endif // AIR_HPP_INCLUDED
