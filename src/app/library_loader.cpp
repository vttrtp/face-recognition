#include "library_loader.hpp"

#include <iostream>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace app {

bool LibraryLoader::loadLibrary(const std::string& library_path, const char* log_prefix) {
    void* raw_handle = loadLibraryImpl(library_path.c_str());
    if (!raw_handle) {
        std::cerr << "[" << log_prefix << "] Failed to load library: " << library_path
                  << "\nError: " << getErrorImpl() << std::endl;
        return false;
    }
    
    handle_ = {raw_handle, closeLibraryImpl};
    return true;
}

void* LibraryLoader::getSymbol(const char* name) const {
    if (!handle_) return nullptr;
    return getSymbolImpl(handle_.get(), name);
}

void* LibraryLoader::loadLibraryImpl(const char* path) {
#ifdef _WIN32
    return LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW);
#endif
}

void LibraryLoader::closeLibraryImpl(void* handle) {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* LibraryLoader::getSymbolImpl(void* handle, const char* name) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}

std::string LibraryLoader::getErrorImpl() {
#ifdef _WIN32
    return "Windows error code: " + std::to_string(GetLastError());
#else
    const char* err = dlerror();
    return err ? err : "Unknown error";
#endif
}

} // namespace app
