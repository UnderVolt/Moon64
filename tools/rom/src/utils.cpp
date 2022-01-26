#include "utils.h"
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

namespace fs = std::filesystem;
using namespace std;

#ifndef CWD
#define CWD "."
#endif

namespace MoonUtils {

#if IS_BIG_ENDIAN
    const char* endian = "big";
#else
    const char* endian = "little";
#endif

#if IS_64_BIT
    const char* bitwidth = "64";
#else
    const char* bitwidth = "32";
#endif

    const char* cwd = NULL;

    char *sys_strdup(const char *src) {
        const unsigned len = strlen(src) + 1;
        char *newstr = new char[len];
        if (newstr) memcpy(newstr, src, len);
        return newstr;
    }

    string normalize(string path){
        replace(path.begin(), path.end(), '\\', '/');
        return path;
    }

    string join(string base, string file){
        if( file == "/" ) return normalize(base + file);
        return normalize( (fs::path(base) / fs::path(file)).string() );
    }

    string dirname(string path){
        return normalize( fs::path(path).parent_path().string());
    }

    string filename(string path){
        return normalize( fs::path(path).filename().string() );
    }

    string basename(string path){
        return normalize( fs::path(path).stem().string() );
    }

    vector<string> split (const string &s, char delim) {
        vector<string> result;
        stringstream ss (s);
        string item;
        while (getline(ss, item, delim)) {
            result.push_back (item);
        }
        return result;
    }

    void bindCwd(){
        string tcwd(CWD);
        cwd = sys_strdup(tcwd.substr(0, tcwd.find_last_of("/")).c_str());
    }

    string exec(string cmd) {
        array<char, 128> buffer;
        string result;
        unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) {
            throw runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    bool exists(string path){
        return fs::exists(path);
    }

    void mkdir(string path){
        fs::create_directories(path);
    }

    void rm(string path){
        if(!fs::exists(path)) return;
        fs::remove_all(path);
    }

    string relative(string parent, string child){
        return normalize(fs::relative(child, parent).string());
    }

    void move(string src, string dst){
    #ifdef __MINGW32__
        if (!MoveFileExA(src.c_str(), dst.c_str(), MOVEFILE_COPY_ALLOWED)) {
            printf ("MoveFileEx failed with error %d\n", GetLastError());
            return;
        }
    #else
        vector<string> trashcan;
        for(auto& p: fs::recursive_directory_iterator(src)){
            // Create path in target, if not existing.
            const auto relativeSrc = fs::relative(p, src);
            const auto targetParentPath = dst / relativeSrc.parent_path();
            fs::create_directories(targetParentPath);
            // Copy to the targetParentPath which we just created.
            fs::copy(p, targetParentPath, fs::copy_options::overwrite_existing);

            string dname = p.path().parent_path().string();
            trashcan.push_back(dname);
        }
       for(auto& p: trashcan) rm(p);
    #endif
    }

    void copy(string src, string dst){
        if(!fs::exists(dirname(dst))) mkdir(dirname(dst));
        fs::copy(src, dst, fs::copy_options::update_existing | fs::copy_options::recursive);
    }

    void write(std::string path, char* data, int size){
        mkdir(dirname(path));
        ofstream outfile(path, ios::out | ios::binary);
        outfile.write(data, size);
        outfile.flush();
        outfile.close();
    }

    void writeFile(string path, string content){
        ofstream out(path);
        out << content;
        out.close();
    }

    void dirscan(string path, vector<string> &files){
        if(!fs::is_directory(path)) return;
        for (auto& p : fs::directory_iterator(path)){
            string fpath = p.path().string();
            files.push_back(fpath);
            dirscan(fpath, files);
        }
    }

    string extname(string file) {
        const char *filename = file.c_str();
        const char *dot = strrchr(filename, '.');
        if(!dot || dot == filename) return "";
        return string(dot + 1);
    }
}