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
