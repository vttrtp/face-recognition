#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace app {

/**
 * @brief Base class for dynamic library loading
 */
class LibraryLoader {
public:
    virtual ~LibraryLoader() = default;
    
    LibraryLoader(const LibraryLoader&) = delete;
    LibraryLoader& operator=(const LibraryLoader&) = delete;
    LibraryLoader(LibraryLoader&&) = delete;
    LibraryLoader& operator=(LibraryLoader&&) = delete;

    [[nodiscard]] bool isLoaded() const noexcept { return handle_ != nullptr; }
    [[nodiscard]] explicit operator bool() const noexcept { return isLoaded(); }

protected:
    LibraryLoader() = default;

    [[nodiscard]] bool loadLibrary(const std::string& library_path, const char* log_prefix = "LibraryLoader");
    [[nodiscard]] void* getSymbol(const char* name) const;

private:
    using HandleDeleter = void(*)(void*);
    std::unique_ptr<void, HandleDeleter> handle_{nullptr, nullptr};

    static void* loadLibraryImpl(const char* path);
    static void closeLibraryImpl(void* handle);
    static void* getSymbolImpl(void* handle, const char* name);
    static std::string getErrorImpl();
};

/**
 * @brief CRTP base for singleton library loaders
 * 
 * Provides common singleton pattern: initialize(), instance(), isInitialized()
 * 
 * Derived class must:
 *   1. Have `static constexpr const char* kLogPrefix = "MyLibrary";`
 *   2. Have private default constructor
 *   3. Implement `bool loadSymbols();`
 *   4. Declare `friend class SingletonLibrary<Derived>;`
 */
template<typename Derived>
class SingletonLibrary : public LibraryLoader {
public:
    static bool initialize(const std::string& library_path) {
        if (instance_) {
            return instance_->isLoaded();
        }
        instance_.reset(new Derived());
        
        if (!instance_->loadLibrary(library_path, Derived::kLogPrefix)) {
            return false;
        }
        
        return instance_->loadSymbols();
    }
    
    static Derived& instance() {
        if (!instance_) {
            throw std::runtime_error(std::string(Derived::kLogPrefix) + 
                " not initialized. Call initialize() first.");
        }
        return *instance_;
    }
    
    static bool isInitialized() noexcept {
        return instance_ != nullptr && instance_->isLoaded();
    }

protected:
    SingletonLibrary() = default;

private:
    static inline std::unique_ptr<Derived> instance_{};
};

} // namespace app

/**
 * @brief X-macro helper for implementing loadSymbols() in derived classes
 * 
 * In your .cpp file, implement loadSymbols() like this:
 * 
 *   bool MyLibrary::loadSymbols() {
 *       #define SYMBOL(name, type, str) \
 *           name = reinterpret_cast<std::add_pointer_t<type>>(getSymbol(str));
 *       #include "my_symbols.inc"
 *       #undef SYMBOL
 *   
 *       bool all_loaded = true
 *       #define SYMBOL(name, type, str) && name
 *       #include "my_symbols.inc"
 *       #undef SYMBOL
 *       ;
 *   
 *       if (!all_loaded) {
 *           std::cerr << "[" << kLogPrefix << "] Failed to load all required symbols" << std::endl;
 *       }
 *       return all_loaded;
 *   }
 */
