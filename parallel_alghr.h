#pragma once

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
#include <numeric>
#include <future>
#include <queue>


/// === accumulate_parallel
template<typename Iterator, typename T>
struct accumulate_block
{
    void operator()(Iterator first, Iterator last, T& result)
    {
        result = std::accumulate(first, last, result);
    }
};

template<typename Iterator, typename T>
T accumulate_parallel(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last);
    if (!length)
        return init;
    unsigned long const min_per_thread = 250;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);
    Iterator block_start = first;
    for (size_t i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i]));
        block_start = block_end;
    }
    accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    return std::accumulate(results.begin(), results.end(), init);

}

/// === transform_parallel

template<typename InputIt, typename OutputIt, typename UnaryOperation>
void transform_parallel(InputIt first1, InputIt last1, OutputIt d_first, UnaryOperation op)
{
    unsigned long const length = std::distance(first1, last1);
    if (!length)
        return;

    unsigned long const min_per_thread = 250;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::vector<std::thread> threads(num_threads - 1);
    InputIt block_start = first1;
    OutputIt result_start = d_first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        InputIt block_end = block_start;
        std::advance(block_end, block_size);
        // create function call in thread: std::transform(block_start, block_end, block_start, op)
        threads[i] = std::thread(std::transform<InputIt, OutputIt, UnaryOperation>, block_start, block_end, result_start, op);
        block_start = block_end;
        std::advance(result_start, block_size);
    }
    std::transform(block_start, last1, result_start, op);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}

//////generat_parallel
template<typename Iterator, typename Func>
void generate_parallel(Iterator first, Iterator last, Func f)
{
    const ptrdiff_t length = std::distance(first, last);
    if (!length)
        return;

    const size_t min_per_thread = 25; 
    const size_t max_threads = (length + min_per_thread - 1) / min_per_thread;
    const size_t hardware_threads = (std::thread::hardware_concurrency() == 0) ? 2 : std::thread::hardware_concurrency(); 
    const size_t num_threads = std::min(hardware_threads, max_threads); 
    const size_t interval = length / num_threads; 

    std::vector<std::thread> threads;
    threads.reserve(num_threads - 1);
    Iterator start = first;
    for (size_t i = 0; i < (num_threads - 1); ++i) // size - 1 main thread
    {
        Iterator end = start;
        std::advance(end, interval);
        threads.emplace_back(std::generate<Iterator, Func>, start, end, f);
        start = end;
    }
    std::generate(start, last, f);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

}


///find parallel
template<typename Iterator, typename Value>
struct find_par
{
    void operator()(Iterator first, Iterator last, Value val, Iterator& res, std::mutex& mute_iter, std::atomic<bool>& flag)
    {
        for (auto it = first; it != last and !flag.load(); ++it)
        {
            if (*it == val)
            {
                flag.store(true); 
                if (mute_iter.try_lock())
                {
                    res = it;
                    mute_iter.unlock(); 
                }
                return;
            }    
        }
    }
};


template<typename Iterator, typename Value>
Iterator find_parallel(Iterator first, Iterator last, Value val)
{
    std::atomic<bool> flag; 
    std::mutex mute_iter; 

    const std::ptrdiff_t length = std::distance(first, last);
    if (!length)
        return last;

    const std::size_t min_per_thread = 50; 
    const std::size_t max_threads = (length + min_per_thread - 1) / min_per_thread;
    const std::size_t harware_conc = (std::thread::hardware_concurrency() == 0) ? 2 : std::thread::hardware_concurrency(); 
    const std::size_t threads_numb = std::min(harware_conc, max_threads); 

    std::vector<std::thread> threads;
    threads.reserve(threads_numb - 1);

    const std::size_t block_size = length / threads_numb;
    Iterator res = last; 
    Iterator start = first;
    for (size_t i = 0; i < (threads_numb - 1); i++)
    {
        Iterator end = start; 
        std::advance(end, block_size);
        threads.emplace_back(find_par<Iterator, Value>(), start, end, val, std::ref(res), std::ref(mute_iter), std::ref(flag));
        start = end;
    }

    find_par<Iterator, Value>()( start, last, val, std::ref(res), std::ref(mute_iter), std::ref(flag));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    return (res == last) ? last : res;
}


template <typename Iterator,typename T> 
void remove_(Iterator first, Iterator last, const T val)
{
    if (first == last)
        return;

    for (auto it = first, it2 = first; it!=last; ++it) 
    {
        if (*it != val) 
        {
            std::iter_swap(it, it2); 
            ++it2;
        }
    }
}

// find par2
template<typename Iterator, typename Value>
struct find_par2
{
    void operator()(Iterator first, Iterator last, const Value val, std::vector<Iterator> & res, std::mutex& mute_iter)
    {
        for (auto it = first; it != last ; ++it)
        {

            if (*it == val)
            {
                std::lock_guard<std::mutex> lock(mute_iter);
                res.push_back(it); 
            }
        }
    }
};

template<typename Iterator, typename Value>
std::vector<Iterator> find_parallel2(Iterator first, Iterator last, Value val) 
{
    std::mutex mute_iter;

    const std::ptrdiff_t length = std::distance(first, last);
    if (!length)
        return {};

    const std::size_t min_per_thread = 50;
    const std::size_t max_threads = (length + min_per_thread - 1) / min_per_thread;
    const std::size_t harware_conc = (std::thread::hardware_concurrency() == 0) ? 2 : std::thread::hardware_concurrency();
    const std::size_t threads_numb = std::min(harware_conc, max_threads); 

    std::vector<Iterator> vec_it;
    vec_it.reserve(threads_numb);
    std::vector<std::thread> threads;
    threads.reserve(threads_numb - 1);

    const std::size_t block_size = length / threads_numb;
    Iterator start = first;

    for (size_t i = 0; i < (threads_numb - 1); i++)
    {
        Iterator end = start;
        std::advance(end, block_size);
        threads.emplace_back(find_par2<Iterator, Value>(), start, end, val, std::ref(vec_it), std::ref(mute_iter));
        start = end;
    }

    find_par2<Iterator, Value>()(start, last, val, std::ref(vec_it), std::ref(mute_iter));
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    vec_it.shrink_to_fit();
    return vec_it; 
}


// max element
template<typename Iterator>
struct max_element_par
{
    void operator()(Iterator first, Iterator last, std::vector<Iterator>& res, std::mutex& mute_iter)
    {
        auto it = std::max_element(first, last); 
        std::lock_guard<std::mutex> lock(mute_iter);
        res.push_back(it);
    }
    
};

template <typename Iterator>
Iterator max_element_parallel(Iterator first, Iterator last)
{
    const std::ptrdiff_t length = std::distance(first, last); 
    if (!length)
        return last;

    std::mutex mute_iter; 
    const size_t min_per_thread = 50; 
    const size_t max_thread = (length + min_per_thread - 1) / min_per_thread; 
    const size_t hardware_conc = std::thread::hardware_concurrency();   
    const size_t numb_threads = std::min((hardware_conc == 0) ? 2 : hardware_conc, max_thread); 
    const size_t size_block = length / numb_threads;

    std::vector<Iterator> results;   
    results.reserve(numb_threads);  
     
    std::vector<std::thread> threads(numb_threads - 1); // --> 1 for main thread;
    Iterator start = first;
    for (size_t i = 0; i < (numb_threads - 1); i++) 
    {
        Iterator end = start;  
        std::advance(end, size_block); 
        threads[i] = std::thread(max_element_par<Iterator>(), start, end, std::ref(results), std::ref(mute_iter));
        start = end;
    }
    max_element_par<Iterator>()(start, last, std::ref(results),std::ref(mute_iter));  
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));   
    auto res = std::max_element(results.begin(), results.end());   
    return *res; 
}


