#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <sfml/Graphics.hpp>

template<typename T>
struct history
{
    std::vector<T> hist;

    void push_back(T t)
    {
        hist.push_back(t);
    }

    T get(int num)
    {
        if(num >= (int)hist.size())
            return T();

        if(num < 0)
            throw "Bad history request";

        auto it = hist.end() - 1;

        for(int i=0; i<num; i++)
        {
            it--;
        }

        return *it;
    }

    void clear()
    {
        hist.clear();
    }

    int size()
    {
        return hist.size();
    }
};

template<sf::Keyboard::Key k>
inline
bool once()
{
    static bool last;

    sf::Keyboard key;

    if(key.isKeyPressed(k) && !last)
    {
        last = true;

        return true;
    }

    if(!key.isKeyPressed(k))
    {
        last = false;
    }

    return false;
}

template<sf::Mouse::Button b>
inline
bool once()
{
    static bool last;

    sf::Mouse m;

    if(m.isButtonPressed(b) && !last)
    {
        last = true;

        return true;
    }

    if(!m.isButtonPressed(b))
    {
        last = false;
    }

    return false;
}

#endif // MISC_H_INCLUDED
