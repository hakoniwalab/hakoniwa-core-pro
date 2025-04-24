#pragma once

#define HAKO_EXPORTS
#ifdef _WIN32
#ifdef HAKO_EXPORTS
#define HAKO_API __declspec(dllexport)
#else
#define HAKO_API __declspec(dllimport)
#endif
#else
#define HAKO_API extern
#endif
