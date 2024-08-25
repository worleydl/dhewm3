#pragma once
#include <functional>

namespace WinInfo {
	int getHostHeight();
	int getHostWidth();
	void runOnAuxThread(std::function<void()> function);
	void requestConfigSave(std::function<void()> function);
}
