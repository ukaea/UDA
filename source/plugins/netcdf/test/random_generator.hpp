#pragma once

#include <random>
#include <type_traits>


namespace uda::plugins::netcdf
{

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    class RandomGenerator 
    {
        using distribution_type = typename std::conditional<std::is_integral<T>::value, std::uniform_int_distribution<T>,
            std::uniform_real_distribution<T>>::type;

        T min = std::numeric_limits<T>().min();
        T max = std::numeric_limits<T>().max();

        std::default_random_engine engine;
        distribution_type distribution;

        public:
        inline RandomGenerator() : distribution(min, max){}

        auto operator()() -> decltype(distribution(engine)) 
        {
            return distribution(engine);
        }

        inline void reseed(int val)
        {
            engine.seed(val);
        }
    };

} // namespace uda::plugins::netcdf
