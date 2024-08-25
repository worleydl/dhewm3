#pragma once

// Hacky fix for when include messes up other code
#ifndef WININFO_SLIM
#include <functional>
#endif

namespace WinInfo {
	int getHostHeight();
	int getHostWidth();

#ifndef WININFO_SLIM
	void runOnAuxThread(std::function<void()> function);
	void requestConfigSave(std::function<void()> function);
#endif
}
