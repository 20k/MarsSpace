#ifndef BYTE_STRUCT_H_INCLUDED
#define BYTE_STRUCT_H_INCLUDED

#include <vector>

struct byte_vector
{
    std::vector<char> ptr;

    template<typename T>
    void push_back(T v)
    {
        char* pv = (char*)&v;

        for(uint32_t i=0; i<sizeof(T); i++)
        {
            ptr.push_back(pv[i]);
        }
    }


    void push_back(const std::vector<char>& v)
    {
        ptr.insert(ptr.end(), v.begin(), v.end());
    }


    void push_back(const byte_vector& vec)
    {
        push_back(vec.ptr);
    }

    std::vector<char> data()
    {
        return ptr;
    }
};


///could have a fetch.get(offset) which does not increment the internal counter
///then at the end we can do internal_counter += offset
///this would remove c++s undefined order of argument function calling problem

struct byte_fetch
{
    std::vector<char> ptr;

    int internal_counter;

    byte_fetch()
    {
        internal_counter = 0;
    }

    template<typename T>
    void push_back(T v)
    {
        char* pv = (char*)&v;

        for(int i=0; i<sizeof(T); i++)
        {
            ptr.push_back(pv[i]);
        }
    }

    void push_back(const std::vector<char>& v)
    {
        ptr.insert(ptr.end(), v.begin(), v.end());
    }

    void push_back(const std::string& str)
    {
        std::copy(str.begin(), str.end(), std::back_inserter(ptr));
    }

    template<typename T>
    T get()
    {
        int prev = internal_counter;

        internal_counter += sizeof(T);

        if(internal_counter > (int)ptr.size())
        {
            std::cout << "Error, invalid bytefetch" << std::endl;

            return T();
        }

        /*//printf("%f\n", *(float*)&ptr[prev]);

        for(int i=prev; i<internal_counter; i+=4)
        {
            //printf("%f\n", *(float*)&ptr[i]);
        }*/

        return *(T*)&ptr[prev];
    }

    void* get(int size)
    {
        int prev = internal_counter;

        internal_counter += size;

        return (void*)&ptr[prev];
    }

    bool valid()
    {
        return internal_counter < (int)ptr.size();
    }
};

#endif // BYTE_STRUCT_H_INCLUDED
