#pragma once

#include "FileHandler.h"
#include "Logger.h"
#include "Math.h"
#include "Timer.h"
#include "ReadWriteRecursiveMutex.h" 
#include "Renderer.h"
#include "AppVariables.h"
#include <deque>

#define WIN_ERROR_MSG(...) Logger::error(__VA_ARGS__, " with Windows Error: \"", get_win_msg(), "\" in File:\"", __FILE__, "\" Line: ", __LINE__, " in ", __func__);
#define ERROR_MSG(...)     Logger::error(__VA_ARGS__, " in File:\"", __FILE__, "\" Line: ", __LINE__, " in ", __func__);
#define ASSERT(x, ...)                       if (!(x)) {  ERROR_MSG(__VA_ARGS__);     __debugbreak(); } 
#define ASSERT_WIN(x,...)                    if (!(x)) {  WIN_ERROR_MSG(__VA_ARGS__); __debugbreak(); }
#define ASSERT_WITH_STATEMENT(x, y, ...)     if (!(x)) {  ERROR_MSG(__VA_ARGS__);     __debugbreak(); y; }
#define ASSERT_WIN_WITH_STATEMENT(x, y, ...) if (!(x)) {  WIN_ERROR_MSG(__VA_ARGS__); __debugbreak(); y; }
#define ASSERT_RETURN_FALSE(x, ...)          if (!(x)) {  ERROR_MSG(__VA_ARGS__);     __debugbreak(); return false; }
#define ASSERT_WIN_RETURN_FALSE(x, ...)      if (!(x)) {  WIN_ERROR_MSG(__VA_ARGS__); __debugbreak(); return false; }
#define ASSERT_RETURN_NULLOPT(x,...)         if (!(x)) {  ERROR_MSG(__VA_ARGS__);     __debugbreak(); return std::nullopt; }
#define ASSERT_WIN_RETURN_NULLOPT(x, ...)    if (!(x)) {  WIN_ERROR_MSG(__VA_ARGS__); __debugbreak(); return std::nullopt; }

template <typename T>
using ThreadSafeVector = ReadWriteThreadSafeClass<std::vector<T>>;

template <typename T>
using ThreadSafeDeque = ReadWriteThreadSafeClass<std::deque<T>>;
