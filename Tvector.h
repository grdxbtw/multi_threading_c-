#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <type_traits>
#include<chrono>
#include <algorithm>
#include <queue>
#include <condition_variable>
#include <stdexcept>
#include<stack>

template<typename T = int> 
class Tvector
{
private:
	std::vector<T> vec;
	mutable std::mutex mute;

	class iterator
	{
	private:
		friend class Tvector<T>;
		typename std::vector<T>::iterator iter;
		std::mutex& mute_iter;

	public:
		iterator(typename std::vector<T>::iterator it, std::mutex& mtx):iter(it),mute_iter(mtx){}

		T& operator*()
		{
			std::lock_guard<std::mutex> lock_push(mute_iter);			
			return *iter;
		}
		iterator& operator++()
		{
			std::lock_guard<std::mutex> lock_push(mute_iter);
			++iter;
			return *this;
		}
	};

public:

	Tvector() = default;

	Tvector(const std::size_t size_)
	{
		vec.size(size_);
	}
	Tvector (std::initializer_list<T> l) : vec(l) 
	{

	}
	Tvector(const Tvector& other)
	{
		std::lock_guard<std::mutex> lock_push(other.mute);
		vec = other.vec;
	}
	Tvector(Tvector&& other)
	{
		std::lock_guard<std::mutex> lock_push(other.mute);
		vec = std::move(other.vec);
	}
	Tvector& operator= (const Tvector& obj)
	{
		std::lock_guard<std::mutex> lock_push(mute);
		if (this != &obj)
		{
			vec = obj.vec; 
		}
		return *this;
	}
	Tvector& operator= ( Tvector&& obj)
	{
		std::lock_guard<std::mutex> lock_push(mute);
		if (this != &obj)
		{
			vec = std::move(obj.vec);
		}
		return *this;
	}
	T operator[](std::size_t index) const
	{
		std::lock_guard<std::mutex> lock_push(mute);
		return vec[index];
	}
	std::size_t size() const 
	{
		std::lock_guard<std::mutex> lock_push(mute);
		return vec.size();
	}
	void resize(std::size_t newsise)
	{
		std::lock_guard<std::mutex> lock_push(mute);
		vec.resize(newsise);
	}
	void push_back(const T& value)
	{
		std::lock_guard<std::mutex> lock_push(mute);
		vec.emplace_back(value);
	}
	template< typename Iterator>
	void insert(iterator where, Iterator begin, Iterator end)
	{
		std::lock_guard<std::mutex> lock_push(mute);
		auto it = where.iter;
		vec.insert(it,begin,end);

	}
	void erase(const std::size_t index)
	{
		std::lock_guard<std::mutex> lock_push(mute);
		auto it = vec.begin() + index;
		vec.erase(it);
	}
	constexpr T get_max() const
	{
		std::lock_guard<std::mutex> lock_push(mute);
		const auto t = std::max_element(vec.begin(), vec.end());
		return *t;
	}
	constexpr T get_min() const
	{
		std::lock_guard<std::mutex> lock_push(mute);
		const auto t = std::min_element(vec.begin(), vec.end());
		return *t;
	}
	iterator begin()
	{
		return iterator(vec.begin(), mute);
	}
	iterator end()
	{
		return iterator(vec.end(), mute);
	}
	void clear()
	{
		std::lock_guard<std::mutex> lock_push(mute);
		vec.clear();
	}
};


template<typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut;  // ћьютекс должен быть измен€емым 
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue()
	{}
	threadsafe_queue(threadsafe_queue const& other)
	{
		std::lock_guard<std::mutex> lk(other.mut);
		data_queue = other.data_queue;
	}
	void push(T new_value)
	{
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(new_value);
		data_cond.notify_one();
	}
	void wait_and_pop(T& value)
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		value = data_queue.front();
		data_queue.pop();
	}
	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lk(mut); 
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		std::shared_ptr<T> res(std::make_shared<T>(data_queue.front())); 
		data_queue.pop();
		return res;
	}
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return false;
		value = data_queue.front();
		data_queue.pop();

		return true;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res(std::make_shared<T>(data_queue.front())); data_queue.pop();
		return res;
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mut);
		return data_queue.empty();
	}
};


struct empty_stack : std::exception
{
	const char* what() const throw();
};

template<typename T>
class threadsafe_stack
{
private:
	std::stack<T> data;
	mutable std::mutex m;
public:
	threadsafe_stack() {}
	threadsafe_stack(const threadsafe_stack& other)
	{
		std::lock_guard<std::mutex> lock(other.m);
		data = other.data;
	}
	threadsafe_stack& operator=(const threadsafe_stack&) = delete;
	void push(T new_value)
	{
		std::lock_guard<std::mutex> lock(m);
		data.push(std::move(new_value)); 
	}
	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) throw empty_stack(); 
			std::shared_ptr<T> const res(
				std::make_shared<T>(std::move(data.top()))); 
		data.pop(); 
		return res;
	}
	void pop(T& value)
	{
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) throw empty_stack();
		value = std::move(data.top()); 
		data.pop();
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lock(m);
		return data.empty();
	}
};

