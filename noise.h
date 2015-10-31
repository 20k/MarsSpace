#ifndef NOISE_H_INCLUDED
#define NOISE_H_INCLUDED

#include <vec/vec.hpp>

inline float noise_2d(int x, int y)
{
    int n=x*271 + y*1999;

    n=(n<<13)^n;

    int nn=(n*(n*n*41333 +53307781)+1376312589)&0x7fffffff;

    return ((1.0-((float)nn/1073741824.0)));// + noise1(x) + noise1(y) + noise1(z) + noise1(w))/5.0;
}

inline float cosine_interpolate(float a, float b, float x)
{
    float ft = x * M_PI;
    float f = ((1.0 - cos(ft)) * 0.5);

    return  (a*(1.0-f) + b*f);
}

inline float linear_interpolate(float a, float b, float x)
{
    return  (a*(1.0-x) + b*x);
    //return cosine_interpolate(a, b, x);
}

inline float interpolate(float a, float b, float x)
{
    return cosine_interpolate(a, b, x);
}


inline float smoothnoise_2d(float fx, float fy)
{
    int x, y;
    x = fx, y = fy;

    float V1=noise_2d(x, y);

    float V2=noise_2d(x+1,y);

    float V3=noise_2d(x, y+1);

    float V4=noise_2d(x+1,y+1);


    float I1=interpolate(V1, V2, fx-int(fx));

    float I2=interpolate(V3, V4, fx-int(fx));

    float I3=interpolate(I1, I2, fy-int(fy));


    return I3;
}

float noisemod_2d(float x, float y, float ocf, float amp)
{
    return smoothnoise_2d(x*ocf, y*ocf)*amp;
}

float noisemult_2d(int x, int y)
{
    float mx, my, mz;

    float mwx = noisemod_2d(x, y, 0.25, 2);
    float mwy = noisemod_2d(mwx*30, y, 0.25, 2); ///

    mx = x + mwx*2;
    my = y + mwy*2;

    float accum = 0;
    float power = 0.f;

    for(int i=0; i<5; i++)
    {
        float frequency = 1.f / pow(2.f, i);

        accum += noisemod_2d(mx, my, frequency, 1.f / frequency);

        power += 1.f / frequency;
    }


    return accum / power;

    //return noisemod_2d(mx, my, 8, 6) + noisemod_2d(mx, my, 2, 6) + noisemod_2d(mx, my, 0.8, 6) + noisemod_2d(mx, my, 0.4, 6) + noisemod_2d(mx, my, 0.1, 6.01) + noisemod_2d(mx, my, 0.174, 4.01) + noisemod_2d(mx, my, 0.067, 2.23) + noisemod_2d(mx, my, 0.0243, 1.00);
    //return noisemod(mx, my, mz, 0, 0.067, 8.23) + noisemod(mx, my, mz, 0, 0.0383, 14.00);
    //return noisemod(mx, my, mz, 0, 0.0383, 14.00);
}



///scrap this macro, its causing issues
#define IX(x, y, z) ((z)*width*height + (y)*width + (x))

vec3f get_wavelet(int x, int y, int z, int width, int height, int depth, float* w1, float* w2, float* w3)
{
    x = x % width;
    y = y % height;
    z = z % depth;

    int x1, y1, z1;

    x1 = (x + width - 1) % width;
    y1 = (y + height - 1) % height;
    z1 = (z + depth - 1) % depth;

    float d1y = w1[IX(x, y, z)] - w1[IX(x, y1, z)];
    float d2z = w2[IX(x, y, z)] - w2[IX(x, y, z1)];

    float d3z = w3[IX(x, y, z)] - w3[IX(x, y, z1)];
    float d1x = w1[IX(x, y, z)] - w1[IX(x1, y, z)];

    float d2x = w2[IX(x, y, z)] - w2[IX(x1, y, z)];
    float d3y = w3[IX(x, y, z)] - w3[IX(x, y1, z)];

    return (vec3f){d1y - d2z, d3z - d1x, d2x - d3y};
}

vec3f get_wavelet_interpolated(float lx, float ly, float lz, int width, int height, int depth, float* w1, float* w2, float* w3)
{
    vec3f v1, v2, v3, v4, v5, v6, v7, v8;

    int x = lx, y = ly, z = lz;

    v1 = get_wavelet(x, y, z, width, height, depth, w1, w2, w3);
    v2 = get_wavelet(x+1, y, z, width, height, depth, w1, w2, w3);
    v3 = get_wavelet(x, y+1, z, width, height, depth, w1, w2, w3);
    v4 = get_wavelet(x+1, y+1, z, width, height, depth, w1, w2, w3);
    v5 = get_wavelet(x, y, z+1, width, height, depth, w1, w2, w3);
    v6 = get_wavelet(x+1, y, z+1, width, height, depth, w1, w2, w3);
    v7 = get_wavelet(x, y+1, z+1, width, height, depth, w1, w2, w3);
    v8 = get_wavelet(x+1, y+1, z+1, width, height, depth, w1, w2, w3);

    vec3f x1, x2, x3, x4;

    float xfrac = lx - floor(lx);

    x1 = mix(v1, v2, xfrac);//add(mult(v1, (1.0f - xfrac)), mult(v2, xfrac));
    x2 = mix(v3, v4, xfrac);//add(mult(v3, (1.0f - xfrac)), mult(v4, xfrac));
    x3 = mix(v5, v6, xfrac);//add(mult(v5, (1.0f - xfrac)), mult(v6, xfrac));
    x4 = mix(v7, v8, xfrac);//add(mult(v7, (1.0f - xfrac)), mult(v8, xfrac));

    float yfrac = ly - floor(ly);

    vec3f y1, y2;

    y1 = mix(x1, x2, yfrac);//add(mult(x1, (1.0f - yfrac)), mult(x2, yfrac));
    y2 = mix(x3, x4, yfrac);//add(mult(x3, (1.0f - yfrac)), mult(x4, yfrac));

    float zfrac = lz - floor(lz);

    return mix(y1, y2, zfrac);//add(mult(y1, (1.0f - zfrac)), mult(y2, zfrac));
}

vec3f y_of(int x, int y, int z, int width, int height, int depth, float* w1, float* w2, float* w3,
            int imin, int imax)
{
    vec3f accum = {0,0,0};

    for(int i=imin; i<imax; i++)
    {
        vec3f new_pos = (vec3f){x, y, z};

        new_pos = new_pos * powf(2.f, (float)i);//mult(new_pos, powf(2.0f, (float)i));

        vec3f w_val = get_wavelet_interpolated(new_pos.v[0], new_pos.v[1], new_pos.v[2], width, height, depth, w1, w2, w3);

        w_val = w_val * powf(2.0f, (-5.0f/6.0f)*(i - imin));

        accum = accum + w_val;//add(accum, w_val);
    }

    return accum;
}

float* noise_buf(int width, int height)
{
    int depth = 4;

    float* tw1, *tw2, *tw3, *ret;

    ///needs to be nw, nh, nd
    tw1 = new float[width*height*depth];
    tw2 = new float[width*height*depth];
    tw3 = new float[width*height*depth];


    ret = new float[width*height];

    for(unsigned int i = 0; i<width*height*depth; i++)
    {
        tw1[i] = randf_s(0.f, 1.f);
        tw2[i] = randf_s(0.f, 1.f);
        tw3[i] = randf_s(0.f, 1.f);
    }

    float min_val = 1000;
    float max_val = -1000;

    for(int z=2; z<=2; z++)
        for(int y=0; y<height; y++)
            for(int x=0; x<width; x++)
    {
        ///figured it out finally
        ///energy bands correspond to frequencies
        ///which correspond to spacial coherence
        ///negative frequencies?
        int imin = -7;
        int imax = 0;

        ///tinker with this
        vec3f val = y_of(x, y, z, width, height, depth, tw1, tw2, tw3, imin, imax);

        if(val.v[0] < min_val)
            min_val = val.v[0];

        if(val.v[0] > max_val)
            max_val = val.v[0];

        //val = fabs(val);

        ret[IX(x, y, 0)] = val.v[0];
    }

    for(int y=0; y<height; y++)
        for(int x=0; x<width; x++)
    {
        ret[IX(x, y, 0)] = (ret[IX(x, y, 0)] - min_val) / (max_val - min_val);
    }

    delete [] tw1;
    delete [] tw2;
    delete [] tw3;

    return ret;
}

float* pnoise_buf(int width, int height)
{
    float* ret = new float[width*height];

    float min_val = 1000;
    float max_val = -1000;

    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            float val = noisemult_2d(x, y);

            if(val < min_val)
                min_val = val;

            if(val > max_val)
                max_val = val;

            ret[y*width + x] = val;
        }
    }

    for(int y=0; y<height; y++)
        for(int x=0; x<width; x++)
    {
        ret[IX(x, y, 0)] = (ret[IX(x, y, 0)] - min_val) / (max_val - min_val);
    }

    return ret;
}

#endif // NOISE_H_INCLUDED
