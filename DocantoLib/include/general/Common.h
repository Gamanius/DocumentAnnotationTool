#ifndef _COMMON_H_
#define _COMMON_H_

#include <string>
#include <optional>
#include <memory>

#include <deque>
#include <map>
#include <vector>

#include <fstream>
#include <filesystem>
#include <functional>

#include <mutex>
#include <shared_mutex>
#include <condition_variable>


typedef unsigned char byte;

namespace Docanto {
	inline std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();
}

#endif