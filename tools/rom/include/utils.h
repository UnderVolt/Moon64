#ifndef FSUtils_H
#define FSUtils_H

#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#ifdef __MINGW32__
#include <windows.h>
#endif

#define IS_64_BIT (UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFU)
#define IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

#define PARSE(val) val.is_string() ? val.get<std::string>() : to_string(val)
#define CONTAINS(str, val) str.find(val) != std::string::npos
#define CNV(val) MoonUtils::sys_strdup(val.c_str())
#define CONST(val) MoonUtils::sys_strdup(val)
#define DISPATCH(func, args) func(sizeof(args) / sizeof(char*), args);

namespace fs = std::filesystem;

#ifndef CWD
#define CWD "."
#endif

namespace MoonUtils {
    extern const char* endian;
    extern const char* bitwidth;
    extern const char* cwd;

    void bindCwd();
    char *sys_strdup(const char *src);
    std::string normalize(std::string path);
    std::string join(std::string base, std::string file);
    std::string dirname(std::string path);
    std::string filename(std::string path);
    std::string basename(std::string path);
    std::string exec(std::string cmd);
    std::vector<std::string> split(const std::string &s, char delim);
    bool exists(std::string path);
    void mkdir(std::string path);
    void rm(std::string path);
    std::string relative(std::string parent, std::string child);
    void move(std::string src, std::string dst);
    void copy(std::string src, std::string dst);
    void write(std::string path, char* data, int size);
    void writeFile(std::string path, std::string content);
    void dirscan(std::string path, std::vector<std::string> &files);
    std::string extname(std::string file);

    template< typename... Args >
    std::string format( const char* format, Args... args ) {
        int length = std::snprintf( nullptr, 0, format, args... );
        assert( length >= 0 );

        char* buf = new char[length + 1];
        std::snprintf( buf, length + 1, format, args... );

        std::string str( buf );
        delete[] buf;
        return str;
    }

}

#endif