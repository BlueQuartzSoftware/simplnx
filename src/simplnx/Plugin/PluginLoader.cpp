#include "simplnx/Plugin/PluginLoader.hpp"

#include <fmt/core.h>

#include <memory>

// fmt >= 8.0.0
#if FMT_VERSION >= 80000
#include <fmt/xchar.h>
#endif

#if defined(_WIN32)
#include <Windows.h>
#include <fmt/format.h>
#define SIMPLNX_TEXT_IMPL(text) L##text
#elif defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#define SIMPLNX_TEXT_IMPL(text) text
#else
static_assert(false, "Plugin loading not implemented on this platform");
#endif

#define SIMPLNX_TEXT(text) SIMPLNX_TEXT_IMPL(text)

#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Plugin/AbstractPlugin.hpp"

using namespace nx::core;

namespace
{
template <class T>
constexpr bool is_function_ptr_v = (std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>);

void* LoadSharedLibrary(const std::filesystem::path& path)
{
#if defined(_WIN32)
  return LoadLibraryW(path.c_str());
#elif defined(__linux__) || defined(__APPLE__)
  return dlopen(path.c_str(), RTLD_NOW);
#endif
}

bool UnloadSharedLibrary(void* handle)
{
#if defined(_WIN32)
  return FreeLibrary(reinterpret_cast<HINSTANCE>(handle)) != 0;
#elif defined(__linux__) || defined(__APPLE__)
  return dlclose(handle) == 0;
#endif
}

template <class Func, class = std::enable_if_t<is_function_ptr_v<Func>>>
Func ImportFunction(void* handle, const char* funcName)
{
#if defined(_WIN32)
  return reinterpret_cast<Func>(GetProcAddress(reinterpret_cast<HINSTANCE>(handle), funcName));
#elif defined(__linux__) || defined(__APPLE__)
  return reinterpret_cast<Func>(dlsym(handle, funcName));
#endif
}

auto GetErrorMessage()
{
#if defined(_WIN32)
  LPWSTR messageBuffer = nullptr;
  DWORD error = GetLastError();
  DWORD result = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);
  if(result == 0)
  {
    return std::wstring(fmt::format(L"Unable to retrieve error message for error code '{}'", error));
  }
  auto deleter = [](LPWSTR ptr) { LocalFree(ptr); };
  std::unique_ptr<WCHAR, decltype(deleter)> uniqueBuffer(messageBuffer, deleter);
  std::wstring message = fmt::format(L"Error code {}: {}", error, messageBuffer);
  return message;
#elif defined(__linux__) || defined(__APPLE__)
  const char* error = dlerror();
  if(error == nullptr)
  {
    return std::string("Unable to retrieve error message");
  }
  std::string message(error);
  return message;
#endif
}
} // namespace

PluginLoader::PluginLoader(const std::filesystem::path& path)
: m_Path(path)
, m_Plugin(nullptr)
{
  loadPlugin();
}

PluginLoader::~PluginLoader() noexcept = default;

void PluginLoader::loadPlugin()
{
  m_Handle = LoadSharedLibrary(m_Path);
  if(m_Handle == nullptr)
  {
    fmt::print(SIMPLNX_TEXT("Could not load library '{}' with the following error:\n"), m_Path.c_str());
    fmt::print(SIMPLNX_TEXT("{}\n"), GetErrorMessage());
    return;
  }

  CreatePluginFunc createPluginFunc = ImportFunction<CreatePluginFunc>(m_Handle, SIMPLNX_CREATE_PLUGIN_FUNC_NAME);
  if(createPluginFunc == nullptr)
  {
    unloadPlugin();
    fmt::print(SIMPLNX_TEXT("Could not retrieve function \"" SIMPLNX_CREATE_PLUGIN_FUNC_NAME "\" from library '{}' with the following error:\n"), m_Path.c_str());
    fmt::print(SIMPLNX_TEXT("{}\n"), GetErrorMessage());
    return;
  }

  DestroyPluginFunc destroyPluginFunc = ImportFunction<DestroyPluginFunc>(m_Handle, SIMPLNX_DESTROY_PLUGIN_FUNC_NAME);
  if(destroyPluginFunc == nullptr)
  {
    unloadPlugin();
    fmt::print(SIMPLNX_TEXT("Could not retrieve function \"" SIMPLNX_DESTROY_PLUGIN_FUNC_NAME "\" from library '{}' with following error:\n"), m_Path.c_str());
    fmt::print(SIMPLNX_TEXT("{}\n"), GetErrorMessage());
    return;
  }

  m_Plugin = std::shared_ptr<AbstractPlugin>(createPluginFunc(), destroyPluginFunc);
}

void PluginLoader::unloadPlugin()
{
  if(!m_Handle)
  {
    return;
  }
  m_Plugin = nullptr;
  if(!UnloadSharedLibrary(m_Handle))
  {
    fmt::print(SIMPLNX_TEXT("The following error occurred while unloading library '{}':\n"), m_Path.c_str());
    fmt::print(SIMPLNX_TEXT("{}\n"), GetErrorMessage());
  }
  m_Handle = nullptr;
}

bool PluginLoader::isLoaded() const
{
  return m_Plugin != nullptr;
}

AbstractPlugin* PluginLoader::getPlugin()
{
  return m_Plugin.get();
}

const AbstractPlugin* PluginLoader::getPlugin() const
{
  return m_Plugin.get();
}
