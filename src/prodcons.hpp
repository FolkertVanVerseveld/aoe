#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <optional>

template<typename T>
class ConcurrentChannel final {
	std::mutex mut;
	std::queue<T> data;
	std::condition_variable can_produce, can_consume;
	std::size_t max;
	T mStop;

	std::atomic_bool running;
public:
	ConcurrentChannel(T mStop, std::size_t max) : mut(), data(), can_produce(), can_consume(), max(max), mStop(mStop), running(true) {}

	constexpr bool is_open() const noexcept { return running.load(); }

	/** Emplace a new T in the FIFO-queue and notify the consumer. */
	template<typename... Args>
	void produce(Args&&... args) {
		std::unique_lock<std::mutex> lock(mut);

		can_produce.wait(lock, [&] {return this->data.size() != max; });
		data.emplace(args...);

		can_consume.notify_one();
	}

	void stop() {
		std::unique_lock<std::mutex> lock(mut);

		running.store(false);
		data.emplace(mStop);

		can_consume.notify_one();
	}

	/** Blocks till a new T has been produced and notifies all producers. */
	T consume() {
		std::unique_lock<std::mutex> lock(mut);

		can_consume.wait(lock, [&] { return !this->data.empty(); });
		T v = data.front();
		data.pop();

		can_produce.notify_all();

		return v;
	}

	/** Tries to get the next produced T and notifies all producers. */
	std::optional<T> try_consume() {
		std::unique_lock<std::mutex> lock(mut);

		if (data.empty())
			return std::nullopt;

		T v = data.front();
		data.pop();

		can_produce.notify_all();

		return std::move(v);
	}
};
