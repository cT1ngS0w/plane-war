#pragma once
#include <string>

#ifdef _WIN32
#include <windows.h>
inline std::string get_exe_dir() {
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (len == 0) return ".";
    std::string path(buf, len);
    auto pos = path.rfind('\\');
    if (pos != std::string::npos) path.resize(pos);
    return path;
}
#else
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/diags.h>
#endif
inline std::string get_exe_dir() {
    char buf[4096];
#ifdef __APPLE__
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        std::string path(buf);
        auto pos = path.rfind('/');
        if (pos != std::string::npos) path.resize(pos);
        return path;
    }
#else
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        std::string path(buf);
        auto pos = path.rfind('/');
        if (pos != std::string::npos) path.resize(pos);
        return path;
    }
#endif
    return ".";
}
#endif
