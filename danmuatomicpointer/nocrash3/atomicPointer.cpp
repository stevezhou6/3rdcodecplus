// atomicPointer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <random>
#include <chrono>
#include <mutex>
#include <atomic>
#include <memory>

class IdanmakuCallback {
public:
	virtual ~IdanmakuCallback() {}
	virtual void OnPush(int nType, const char* pStrMessage, const char* pStrMessageType) = 0;
};

std::atomic<IdanmakuCallback*> g_danmaku_callback{ nullptr };

void SetdanmakuCallback(std::shared_ptr<IdanmakuCallback> pIdanmakuCallBack) {
	g_danmaku_callback.store(pIdanmakuCallBack.get(), std::memory_order_release);
	//std::cout << "SetdanmakuCallback" << std::endl;
}

class DanmakuCallback : public IdanmakuCallback {
public:
	void OnPush(int nType, const char* pStrMessage, const char* pStrMessageType) override {
		//std::cout << "OnPush():" << pStrMessage << std::endl;
	}

	~DanmakuCallback() {
		g_danmaku_callback.store(nullptr, std::memory_order_release);
		//std::cout << "~DanmakuCallback()" << std::endl;
	}
};

class RandomStringGenerator {
public:
	RandomStringGenerator()
		: charset("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"),
		generator(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())),
		distribution(0, charset.size() - 1) {}

	std::string getData(size_t length) {
		std::string randomString;
		randomString.reserve(length);
		for (size_t i = 0; i < length; ++i) {
			randomString += charset[distribution(generator)];
		}
		return randomString;
	}

private:
	const std::string charset;
	std::mt19937 generator;
	std::uniform_int_distribution<> distribution;
};

class DYSocketDanmuAdaptor {
public:
	DYSocketDanmuAdaptor() {
		danmuCallbackSptr_ = std::make_shared<DanmakuCallback>();
		SetdanmakuCallback(danmuCallbackSptr_);
	}

	~DYSocketDanmuAdaptor() {
		SetdanmakuCallback(nullptr);
	}

private:
	std::shared_ptr<DanmakuCallback> danmuCallbackSptr_;
};

//crash 
void test() {
	uint64_t pushCount = 0;
	uint64_t createCount = 0;
	std::vector<std::thread> threadsDanmu{ 1 };
	std::atomic<bool> done{ false };
	for (auto& th : threadsDanmu) {
		th = std::thread([&done, &pushCount] {
			RandomStringGenerator randStr;
			while (!done.load(std::memory_order_relaxed)) {
				auto data = randStr.getData(5);
				IdanmakuCallback* callback = g_danmaku_callback.load(std::memory_order_acquire);
				if (callback) {
					callback->OnPush(1, data.c_str(), "");
					++pushCount;
				}
			}
			});
	}

	std::vector<std::thread> threadsApp{ 1 };
	std::atomic<bool> doneApp{ false };
	for (auto& th : threadsApp) {
		th = std::thread([&doneApp, &createCount] {
			while (!doneApp.load(std::memory_order_relaxed)) {
				DYSocketDanmuAdaptor adaptor;
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
				++createCount;
			}
			});
	}

	std::this_thread::sleep_for(std::chrono::seconds(60));

	done.store(true, std::memory_order_relaxed);
	doneApp.store(true, std::memory_order_relaxed);

	for (auto& th : threadsDanmu) {
		th.join();
	}
	for (auto& th : threadsApp) {
		th.join();
	}
	std::cout << " pushCount:" << pushCount << std::endl;
	std::cout << " createCount:" << createCount << std::endl;
}

int main() {
	for (int i = 0; i < 5; i++) {
		test();
	}

	std::cout << "Hello World!\n";
	return 0;
}
