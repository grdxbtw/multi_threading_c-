// paralell_accumulate.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#if defined _MSVC_LANG
#define LANG_VER _MSVC_LANG
#else
#define  LANG_VER __cplusplus
#endif // _MSVC_LANG
#if LANG_VER < 201703L
#error "C++17 needed!"
#endif

#include <execution>
#define SEQ std::execution::seq
#define PAR std::execution::par

#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <chrono>
#include <vector>
#include <algorithm>
#include <utility>
#include <thread>
#include <functional>
#include<numeric>
#include <future>
#include <queue>
#include <optional>
#include <atomic>
#include <condition_variable>

// boost 
#include <boost/regex.hpp>
#include <boost/timer/timer.hpp>

//// header
#include "parallel_alghr.h"


/// === helpers
class Timer
{
public:
    Timer()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    ~Timer()
    {
        auto  end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "timer: " << duration.count() << "ms" << '\n';
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> start;
};


double sqrtd(double val)
{
    return sqrt(val);
}


/// === main
int main()
{
    using namespace std; 

    std::cout << std::fixed << std::setprecision(1);
    const size_t data_size = 150'000'007u;

    std::default_random_engine eng; 
    std::uniform_real_distribution<> urg(2, 50); 

    auto rnd = std::bind(urg, eng); 
    srand(std::time(nullptr));

    std::vector<int> vext{1, 2, 3, 6, 0, 8, 0, 9, 7, 0, 7};
    int val = 0;
    remove_(vext.begin(), vext.end(), val); 

    //////find parralel mine
    {
        vector<int> dvec(data_size);
        generate_parallel( dvec.begin(), dvec.end(), rand);  

        boost::timer::auto_cpu_timer t(3, "%w sec wall;  %t sec cpu (%p%)\n");
        auto it_m = max_element_parallel(dvec.begin(), dvec.end()); 

        auto it = find_parallel(dvec.begin(), dvec.end(), 9);    
        if (it != dvec.end())
            std::cout << *it << std::endl; 
    }


    //////find_par 
    {

        vector<int> dvec(data_size);
        generate_parallel(dvec.begin(), dvec.end(), rand); 
        boost::timer::auto_cpu_timer t(3, "%w sec wall;  %t sec cpu (%p%)\n");
        auto it = find(PAR, dvec.begin(), dvec.end(), rand()); 
        if (it != dvec.end()) 
            std::cout << *it << std::endl; 
    }

    ////generate
    {
        boost::timer::auto_cpu_timer t;
        Timer m;
        vector<double> dvec(data_size);
        generate_parallel( dvec.begin(), dvec.end(), rnd);
        std::cout << "===== Own implementation of parallel generate ==========\n";
    }
    {
        boost::timer::cpu_timer t;
        Timer m;
        vector<double> dvec(data_size);
        generate(PAR, dvec.begin(), dvec.end(), rnd);

        std::cout << "===== PAR generate from algorithm ==========\n";
        boost::timer::cpu_times end = t.elapsed();
        std::cout << boost::timer::format(end)<< '\n';
        t.stop(); 
        if (t.is_stopped()) 
            std::cout << t.format(2, " %t sec CPU, %w sec real// and user_t: ") << t.elapsed().user << '\n'; 

    }

    //////transform
    {
        vector<double> dvec(data_size);
        generate(PAR, dvec.begin(), dvec.end(), rnd);
        Timer m;
        boost::timer::auto_cpu_timer t(3,"%w real sec wall,%t CPU sec (%p%)\n");
        transform(SEQ, dvec.cbegin(), dvec.cend(), dvec.begin(), sqrtd);
        std::cout << "===== SEQ transform from algorithm ==========\n";
    }
    {
        vector<double> dvec(data_size);
        generate(PAR, dvec.begin(), dvec.end(), rnd);
        Timer m;
        transform(PAR, dvec.cbegin(), dvec.cend(), dvec.begin(), sqrtd);
        std::cout << "===== PAR transform from algorithm ==========\n";
    }

    {
        vector<double> dvec(data_size);
        generate(PAR,dvec.begin(), dvec.end(), rnd);
        Timer m;
        transform_parallel(dvec.begin(), dvec.end(), dvec.begin(), sqrtd);
        std::cout << "===== Own implementation of parallel transform ==========\n";
    }
    std::cout << endl;


   ///par algorithm
    std::cout.imbue(std::locale("en_US.UTF-8"));
    auto eval = [](auto fun)
    {
        const auto t1 = std::chrono::high_resolution_clock::now();
        const auto [name, result] = fun();
        const auto t2 = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::milli> ms = t2 - t1;
        std::cout << std::setw(28) << std::left << name << "\tsum: " << result << "\ttime: " << ms.count() << " ms\n";
    };
    
    {
        const std::vector<double> v(data_size, 0.1);

        eval(
            [&v] { return make_pair("std::accumulate (double)", std::accumulate(v.cbegin(), v.cend(), 0.0)); } 
        );
        eval(
            [&v] { return make_pair("std::reduce (seq, double)", std::reduce(SEQ, v.cbegin(), v.cend())); } 
        );
        eval(
            [&v] { return make_pair("std::reduce (par, double)", std::reduce(PAR, v.cbegin(), v.cend())); } 
        );
        eval(
            [&v] { return make_pair("accumulate_parallel (double)", accumulate_parallel(v.cbegin(), v.cend(), 0.0)); } 
        );
    }

    std::cout << endl;
    {
        const std::vector<long> v(data_size, 1); 

        eval(
            [&v] { return make_pair("std::accumulate (long)", std::accumulate(v.cbegin(), v.cend(), 0l)); }
        );
        eval(
            [&v] { return make_pair("std::reduce (seq, long)", std::reduce(SEQ, v.cbegin(), v.cend())); }
        );
        eval(
            [&v] { return make_pair("std::reduce (par, long)", std::reduce(PAR, v.cbegin(), v.cend())); }
        );
        eval(
            [&v] { return make_pair("accumulate_parallel (long)", accumulate_parallel(v.cbegin(), v.cend(), 0l)); }
        );
    }


    return 0;
}

