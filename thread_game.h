#pragma once

#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>

struct rnd_uniform
{
	std::reference_wrapper<std::random_device> rd;

	explicit rnd_uniform(std::random_device& gen) : rd(gen) {}

	int operator()(const std::int16_t min, const std::int16_t max)
	{
		std::uniform_int_distribution<> uid(min, max);
		return uid(rd.get());
	}
};

int Quantity;
std::mutex Quantity_mtx;

/// Using funtion

void playre_thread(int player_id, std::condition_variable& cv_self, std::condition_variable& cv_next, rnd_uniform rnd)
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(Quantity_mtx);
			cv_self.wait(lock);

			if (Quantity < 1)
				break;
			if (Quantity == 1)
			{
				std::cout << "Looser:\t" << std::this_thread::get_id() << "\t#" << player_id << '\n';
				Quantity = 0;
				break;
			}
			auto d = rnd(1, (Quantity <= 10) ? Quantity / 2 : 10);
			std::cout << std::this_thread::get_id() << "\t#" << player_id << " Quantity: " << Quantity << " - " << d << '\n';
			Quantity -= d;
		}
		cv_next.notify_one();
	}
	cv_next.notify_one();
}


void game_via_function()
{
	Quantity = 25;
	std::random_device rnd_dev;
	rnd_uniform rnd_gen(rnd_dev);
	int players_num = 3;
	std::vector<std::condition_variable> arr_cv(players_num);
	std::vector<std::thread> players;
	for (size_t i = 0; i < players_num; i++)
	{
		players.emplace_back(playre_thread, i + 1, std::ref(arr_cv[i]), std::ref(arr_cv[(i + 1) % players_num]), rnd_gen);
	}

	std::cout << "Game started!\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	arr_cv[0].notify_one();

	for (std::thread& th : players)
		th.join();
}


/// Usind objects
class Player
{
private:
	size_t id;
	std::condition_variable* cv_self;
	std::condition_variable* cv_next;
	rnd_uniform rnd;
	std::thread t;

	void game()
	{
		while (true)
		{
			{
				std::unique_lock<std::mutex> lock(Quantity_mtx);
				cv_self->wait(lock);
				if (Quantity < 1)
					break;
				if (Quantity == 1)
				{
					std::cout << "Looser:\t" << std::this_thread::get_id() << "\t#" << id << '\n';
					Quantity = 0;
					break;
				}
				auto d = rnd(1, (Quantity <= 10) ? Quantity / 2 : 10);
				std::cout << std::this_thread::get_id() << "\t#" << id << " Quantity: " << Quantity << " -" << d << '\n';
				Quantity -= d;
			}
			cv_next->notify_one();
		}
		cv_next->notify_one();
	}

public:
	Player(size_t ind, std::condition_variable* cv_current, std::condition_variable* cv_nextplayer, rnd_uniform gen) :
		id(ind), cv_self(cv_current), cv_next(cv_nextplayer), rnd(gen)
	{
		// DDon't run a thread here if the object will be copied or moved, i.e. the "this" address will change.
		// t = std::thread(&Player::game, this);
	}

	Player(Player&& src) = default;

	void run()
	{
		t = std::thread(&Player::game, this);
	}

	void wait()
	{
		if (t.joinable())
			t.join();
	}

	~Player()
	{
		wait();
	}

};


void game_via_object()
{
	Quantity = 25;
	std::random_device rnd_dev;
	rnd_uniform rnd_gen(rnd_dev);
	int players_num = 3;
	std::vector<std::condition_variable> arr_cv(players_num);
	{
		std::vector<Player> players;
		//players.reserve(players_num); 
		for (size_t i = 0; i < players_num; i++)
		{
			players.emplace_back(i + 1, &arr_cv[i], &arr_cv[(i + 1) % players_num], rnd_gen);
		}

		std::cout << "Game started!\n";
		for (size_t i = 0; i < players_num; i++)
			players[i].run();

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		arr_cv[0].notify_one();
	}
}


