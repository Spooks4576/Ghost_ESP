// MIT License

// Copyright (c) 2020 Adam Kecskes

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Created by Adam Kecskes
// https://github.com/K-Adam/SafeQueue

#pragma once

#include <mutex>
#include <condition_variable>

#include <queue>
#include <utility>

template<class T>
class SafeQueue {

	std::queue<T> q;

	std::mutex mtx;
	std::condition_variable cv;

	std::condition_variable sync_wait;
	bool finish_processing = false;
	int sync_counter = 0;

	void DecreaseSyncCounter() {
		if (--sync_counter == 0) {
			sync_wait.notify_one();
		}
	}

public:

	typedef typename std::queue<T>::size_type size_type;

	SafeQueue() {}

	~SafeQueue() {
		Finish();
	}

	void Produce(T&& item) {

		std::lock_guard<std::mutex> lock(mtx);

		q.push(std::move(item));
		cv.notify_one();

	}

	size_type Size() {

		std::lock_guard<std::mutex> lock(mtx);

		return q.size();

	}

	[[nodiscard]]
	bool Consume(T& item) {

		std::lock_guard<std::mutex> lock(mtx);

		if (q.empty()) {
			return false;
		}

		item = std::move(q.front());
		q.pop();

		return true;

	}

	[[nodiscard]]
	bool ConsumeSync(T& item) {

		std::unique_lock<std::mutex> lock(mtx);

		sync_counter++;

		cv.wait(lock, [&] {
			return !q.empty() || finish_processing;
		});

		if (q.empty()) {
			DecreaseSyncCounter();
			return false;
		}

		item = std::move(q.front());
		q.pop();

		DecreaseSyncCounter();
		return true;

	}

	void Finish() {

		std::unique_lock<std::mutex> lock(mtx);

		finish_processing = true;
		cv.notify_all();

		sync_wait.wait(lock, [&]() {
			return sync_counter == 0;
		});

		finish_processing = false;

	}

};
