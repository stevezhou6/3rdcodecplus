// atomicPointer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <random>
#include <chrono>
class IdanmakuCallback {
public:
	virtual ~IdanmakuCallback() {}
	virtual void OnPush(int nType, const char* pStrMessage, const char* pStrMessageType) = 0;
};

IdanmakuCallback* g_danmaku_callback = NULL;
void SetdanmakuCallback(IdanmakuCallback* pIdanmakuCallBack) {
	g_danmaku_callback = pIdanmakuCallBack;
	std::cout << "SetdanmakuCallback" << std::endl;
}

class DanmakuCallback :public IdanmakuCallback
{
public:
	DanmakuCallback() {

	}
	void OnPush(int nType, const char* pStrMessage, const char* pStrMessageType) {
		std::cout << "OnPush():" << pStrMessage << std::endl;
	}
	~DanmakuCallback() {
		SetdanmakuCallback(nullptr);
		std::cout << "~DanmakuCallback()" << std::endl;
	}
private:
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

int main()
{	
	std::vector<std::thread> threadsDanmu{ 1 };
	std::atomic<bool> done{ false };
	for (auto& th : threadsDanmu) {
		th = std::thread([&done] {
			RandomStringGenerator randStr;
			while (!done.load(std::memory_order_relaxed)) {
				auto data = randStr.getData(5);
				if (g_danmaku_callback)
				{
					g_danmaku_callback->OnPush(1, data.c_str(), "");
				}
			}
			});
	}

	std::vector<std::thread> threadsApp{ 1 };
	std::atomic<bool> doneApp{ false };
	for (auto& th : threadsApp) {
		th = std::thread([&doneApp] {
			while (!doneApp.load(std::memory_order_relaxed)) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(1));
				auto callbackSptr = std::make_shared<DanmakuCallback>();
				SetdanmakuCallback(callbackSptr.get());
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

	std::cout << "Hello World!\n";
}
