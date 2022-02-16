#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
//#define DEBUG
#ifdef DEBUG
#define DEBUG_INFO(format, ...) printf(format, ##__VA_ARGS__);
#else
#define DEBUG_INFO(format, ...)
#endif
#endif