#ifndef PRECOMPILED_H
#define PRECOMPILED_H

#include <stdio.h>
#ifdef _WIN32
#include <SDKDDKVer.h>
#include <tchar.h>
#include "../include/Win32/db_cxx.h"
#else
#include "../include/UNIX/db_cxx.h"
#endif

#ifdef _WIN32
#include <Windows.h>
#define MAKE_DIR(x) CreateDirectory(L##x, NULL)
#define SLEEP(x) Sleep(x)
#define PAUSE_STRING "pause"
#else
#include <sys/stat.h>
#define MAKE_DIR(x) mkdir(x, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define SLEEP(x) usleep(x*1000)
#define PAUSE_STRING "read -p \"Press any key to continue ...\" -n 1"
#endif

#include <memory>
#include <locale>
#include <string>
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <cstdio>
#include "../include/hiredis.h"
#include <sstream>

#endif // PRECOMPILED_H
