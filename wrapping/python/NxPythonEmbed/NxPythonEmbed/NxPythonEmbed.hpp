#pragma once

#include "simplnx/SimplnxPython.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <pybind11/embed.h>

#include <fmt/format.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace nx::python
{
namespace py = pybind11;

class PyOutputRedirect
{
public:
  PyOutputRedirect()
  {
    auto sys = py::module_::import("sys");
    m_Stdout = sys.attr("stdout");
    m_Stderr = sys.attr("stderr");
    auto stringio = py::module_::import("io").attr("StringIO");
    m_StdoutBuffer = stringio();
    m_StderrBuffer = stringio();
    sys.attr("stdout") = m_StdoutBuffer;
    sys.attr("stderr") = m_StderrBuffer;
  }

  PyOutputRedirect(const PyOutputRedirect&) = delete;
  PyOutputRedirect(PyOutputRedirect&&) noexcept = default;

  PyOutputRedirect& operator=(const PyOutputRedirect&) = delete;
  PyOutputRedirect& operator=(PyOutputRedirect&&) noexcept = default;

  ~PyOutputRedirect() noexcept
  {
    try
    {
      auto sys = py::module_::import("sys");
      if(m_Stdout)
      {
        sys.attr("stdout") = m_Stdout;
      }
      if(m_Stderr)
      {
        sys.attr("stderr") = m_Stderr;
      }
    } catch(py::error_already_set& pyException)
    {
      pyException.discard_as_unraisable(__func__);
    }
  }

  std::string getStdoutString()
  {
    m_StdoutBuffer.attr("seek")(0);
    return py::str(m_StdoutBuffer.attr("read")());
  }

  std::string getStderrString()
  {
    m_StderrBuffer.attr("seek")(0);
    return py::str(m_StderrBuffer.attr("read")());
  }

private:
  py::object m_Stdout;
  py::object m_Stderr;
  py::object m_StdoutBuffer;
  py::object m_StderrBuffer;
};

inline py::module_ LoadPythonModuleFromString(const std::string& modName, std::string_view strData, const std::string& filename = "<string>")
{
  auto importLibUtil = py::module_::import("importlib.util");
  auto spec = importLibUtil.attr("spec_from_loader")(modName, py::none());
  auto mod = importLibUtil.attr("module_from_spec")(spec);
  auto builtins = py::module_::import("builtins");
  auto code = builtins.attr("compile")(strData, filename, "exec");
  builtins.attr("exec")(code, mod.attr("__dict__"));
  auto sys = py::module_::import("sys");
  sys.attr("modules")[modName.c_str()] = mod;
  return mod;
}

class ManualImportFinderHolder
{
public:
  ManualImportFinderHolder() = default;

  static ManualImportFinderHolder Create()
  {
    auto simplnxModule = py::module_::import(SIMPLNX_PYTHON_MODULE);
    return ManualImportFinderHolder(simplnxModule.attr("ManualImportFinder")());
  }

  bool isNone() const
  {
    return m_Finder.is_none();
  }

  const py::object& getPyObject() const
  {
    return m_Finder;
  }

  void insert(const std::string& pluginPath)
  {
    m_Finder.attr("insert")(pluginPath);
  }

  void addToMetaPath()
  {
    auto sys = py::module_::import("sys");
    py::list(sys.attr("meta_path")).append(m_Finder);
  }

  void clear()
  {
    m_Finder.attr("clear")();
  }

  static ManualImportFinderHolder GetFromMetaPath()
  {
    auto sys = py::module_::import("sys");
    auto simplnxModule = py::module_::import(SIMPLNX_PYTHON_MODULE);
    py::handle manualImportFinderType = simplnxModule.attr("ManualImportFinder");
    auto metaPath = py::list(sys.attr("meta_path"));
    auto iter = std::find_if(metaPath.begin(), metaPath.end(), [manualImportFinderType](py::handle item) { return py::isinstance(item, manualImportFinderType); });
    if(iter == metaPath.end())
    {
      throw std::runtime_error("ManualImportFinder is not in sys.meta_path");
    }
    return ManualImportFinderHolder(py::reinterpret_borrow<py::object>(*iter));
  }

private:
  ManualImportFinderHolder(py::object obj)
  : m_Finder(std::move(obj))
  {
  }

  py::object m_Finder;
};

inline std::string GetPythonPluginNameFromPath(const std::filesystem::path& path)
{
  return path.stem().string();
}

inline std::set<std::string> GetPythonPluginListFromEnvironment()
{
  auto* var = std::getenv("SIMPLNX_PYTHON_PLUGINS");
  if(var == nullptr)
  {
    return {};
  }

#if defined(_WIN32)
  std::vector<std::string> pluginList = nx::core::StringUtilities::split(var, ';');
#else
  std::vector<std::string> pluginList = nx::core::StringUtilities::split(var, ':');
#endif

  return std::set<std::string>(std::make_move_iterator(pluginList.begin()), std::make_move_iterator(pluginList.end()));
}

using OutputCallback = std::function<void(const std::string&)>;

inline void SetupPythonEnvironmentVars(OutputCallback outputCallback = {})
{
  constexpr const char k_PYTHONHOME[] = "PYTHONHOME";
  constexpr const char k_CONDA_PREFIX[] = "CONDA_PREFIX";

  std::string condaPrefix;
  char* condaPrefixPtr = std::getenv(k_CONDA_PREFIX);
  if(condaPrefixPtr != nullptr)
  {
    condaPrefix = condaPrefixPtr;
    if(outputCallback)
    {
      outputCallback(fmt::format("CONDA_PREFIX={}\n", condaPrefix));
    }
  }

  std::string pythonHome;
  char* pythonHomePtr = std::getenv(k_PYTHONHOME);
  if(pythonHomePtr != nullptr)
  {
    pythonHome = pythonHomePtr;
    if(outputCallback)
    {
      outputCallback(fmt::format("PYTHONHOME={}\n", pythonHome));
    }
  }

  if(pythonHome.empty() && !condaPrefix.empty())
  {
    std::string envVar = fmt::format("{}={}", k_PYTHONHOME, condaPrefix);
    putenv(envVar.data());
    if(outputCallback)
    {
      outputCallback(envVar);
    }
  }
}

enum class ExceptionType
{
  Cpp,
  Python
};

inline std::string ExceptionTypeToString(ExceptionType type)
{
  switch(type)
  {
  case nx::python::ExceptionType::Cpp: {
    return "C++";
  }
  case nx::python::ExceptionType::Python: {
    return "Python";
  }
  }
  return "<unknown>";
}

struct PluginLoadErrorInfo
{
  ExceptionType type;
  std::string pluginName;
  std::string message;
};
using PluginLoadErrorCallback = std::function<void(const PluginLoadErrorInfo&)>;

struct PythonErrorInfo
{
  ExceptionType type;
  std::string message;
};
using PythonErrorCallback = std::function<void(const PythonErrorInfo&)>;

inline std::vector<std::string> LoadPythonPlugins(const std::set<std::string>& pythonPlugins, OutputCallback outputCallback = {}, PluginLoadErrorCallback pluginLoadErrorCallback = {},
                                                  PythonErrorCallback pythonErrorCallback = {})
{
  std::vector<std::string> loadedPythonPlugins;
  try
  {
    auto simplnxModule = py::module_::import(SIMPLNX_PYTHON_MODULE);

    ManualImportFinderHolder manualImportFinder = ManualImportFinderHolder::GetFromMetaPath();

    if(outputCallback)
    {
      outputCallback(fmt::format("Loading Python plugins: {}", pythonPlugins));
    }

    for(const auto& pluginPath : pythonPlugins)
    {
      std::string pluginName = GetPythonPluginNameFromPath(pluginPath);
      if(outputCallback)
      {
        outputCallback(fmt::format("Attempting to load Python plugin: '{}' from '{}'", pluginName, pluginPath));
      }
      try
      {
        manualImportFinder.insert(pluginPath);
        auto mod = py::module_::import(pluginName.c_str());
        simplnxModule.attr("load_python_plugin")(mod);
        if(outputCallback)
        {
          outputCallback(fmt::format("Successfully loaded Python plugin '{}' from '{}'", pluginName, pluginPath));
        }
      } catch(const py::error_already_set& exception)
      {
        if(pluginLoadErrorCallback)
        {
          pluginLoadErrorCallback(PluginLoadErrorInfo{ExceptionType::Python, pluginName, exception.what()});
        }
      } catch(const std::exception& exception)
      {
        if(pluginLoadErrorCallback)
        {
          pluginLoadErrorCallback(PluginLoadErrorInfo{ExceptionType::Cpp, pluginName, exception.what()});
        }
      }
      loadedPythonPlugins.push_back(pluginName);
    }
  } catch(const py::error_already_set& exception)
  {
    if(pythonErrorCallback)
    {
      pythonErrorCallback(PythonErrorInfo{ExceptionType::Python, exception.what()});
    }
  } catch(const std::exception& exception)
  {
    if(pythonErrorCallback)
    {
      pythonErrorCallback(PythonErrorInfo{ExceptionType::Cpp, exception.what()});
    }
  }
  return loadedPythonPlugins;
}

inline py::module_ GetImportlibMetadata()
{
  auto sys = py::module_::import("sys");
  if(sys.attr("version_info").attr("__lt__")(py::make_tuple(3, 10)).cast<bool>())
  {
    return py::module_::import("importlib_metadata");
  }
  else
  {
    return py::module_::import("importlib.metadata");
  }
}

class PluginEntrypoint
{
public:
  PluginEntrypoint(py::object object)
  : m_Entrypoint(object)
  {
  }

  py::object& getPyObject()
  {
    return m_Entrypoint;
  }

  const py::object& getPyObject() const
  {
    return m_Entrypoint;
  }

  std::string getValue() const
  {
    return m_Entrypoint.attr("value").cast<std::string>();
  }

  py::object load()
  {
    return m_Entrypoint.attr("load")();
  }

private:
  py::object m_Entrypoint;
};

inline std::vector<PluginEntrypoint> GetInstalledPythonPlugins()
{
  using namespace pybind11::literals;
  py::module_ importlibMetadata = GetImportlibMetadata();
  auto discoveredPlugins = importlibMetadata.attr("entry_points")("group"_a = "simplnx.plugins");
  std::vector<PluginEntrypoint> plugins;
  for(py::handle pluginEntryPoint : discoveredPlugins)
  {
    plugins.emplace_back(py::reinterpret_borrow<py::object>(pluginEntryPoint));
  }
  return plugins;
}

inline std::vector<std::string> LoadInstalledPythonPlugins(OutputCallback outputCallback = {}, PluginLoadErrorCallback pluginLoadErrorCallback = {}, PythonErrorCallback pythonErrorCallback = {})
{
  std::vector<std::string> loadedPythonPlugins;

  try
  {
    auto simplnxModule = py::module_::import(SIMPLNX_PYTHON_MODULE);
    std::vector<PluginEntrypoint> discoveredPlugins = GetInstalledPythonPlugins();
    if(outputCallback)
    {
      std::set<std::string> pluginNames;
      for(auto& pluginEntryPoint : discoveredPlugins)
      {
        pluginNames.insert(pluginEntryPoint.getValue());
      }
      outputCallback(fmt::format("Loading installed Python plugins: {}", pluginNames));
    }
    for(auto& pluginEntryPoint : discoveredPlugins)
    {
      std::string pluginName = pluginEntryPoint.getValue();
      if(outputCallback)
      {
        outputCallback(fmt::format("Attempting to load installed Python plugin: '{}'", pluginName));
      }
      try
      {
        py::object mod = pluginEntryPoint.load();
        simplnxModule.attr("load_python_plugin")(mod);
        if(outputCallback)
        {
          outputCallback(fmt::format("Successfully loaded installed Python plugin '{}'", pluginName));
        }
        loadedPythonPlugins.push_back(pluginName);
      } catch(const py::error_already_set& exception)
      {
        if(pluginLoadErrorCallback)
        {
          pluginLoadErrorCallback(PluginLoadErrorInfo{ExceptionType::Python, pluginName, exception.what()});
        }
      } catch(const std::exception& exception)
      {
        if(pluginLoadErrorCallback)
        {
          pluginLoadErrorCallback(PluginLoadErrorInfo{ExceptionType::Cpp, pluginName, exception.what()});
        }
      }
    }
  } catch(const py::error_already_set& exception)
  {
    if(pythonErrorCallback)
    {
      pythonErrorCallback(PythonErrorInfo{ExceptionType::Python, exception.what()});
    }
  } catch(const std::exception& exception)
  {
    if(pythonErrorCallback)
    {
      pythonErrorCallback(PythonErrorInfo{ExceptionType::Cpp, exception.what()});
    }
  }

  return loadedPythonPlugins;
}
} // namespace nx::python
