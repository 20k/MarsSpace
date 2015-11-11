#ifndef PLANET_GEN_H_INCLUDED
#define PLANET_GEN_H_INCLUDED

#include "noise.h"
#include <SFML/Graphics.hpp>

struct planet_gen
{
    sf::Texture get_tex(int width, int height)
    {
        float* noise = pnoise_buf(width, height);

        sf::Image img;
        img.create(width, height);

        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                float val = noise[y*width + x];

                val = clamp(val, 0.f, 1.f);

                ///131 79 42 is mars

                vec3f mars = (vec3f){131, 79, 42};

                vec3f res = mars * val;

                img.setPixel(x, y, sf::Color(res.v[0], res.v[1], res.v[2]));
            }
        }

        delete [] noise;

        sf::Texture tex;
        tex.loadFromImage(img);
        tex.setSmooth(true);

        return tex;
    }

    sf::Texture get_iron_tex(int width, int height)
    {
        float* noise = pnoise_iron(width, height);

        sf::Image img;
        img.create(width, height);

        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                float val = noise[y*width + x];

                val = clamp(val, 0.f, 1.f);

                ///131 79 42 is mars

                vec3f res = val * 255.f;

                img.setPixel(x, y, sf::Color(res.v[0], res.v[1], res.v[2]));
            }
        }

        delete [] noise;

        sf::Texture tex;
        tex.loadFromImage(img);
        tex.setSmooth(true);

        return tex;
    }
};

#endif // PLANET_GEN_H_INCLUDED
