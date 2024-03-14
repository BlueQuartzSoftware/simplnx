#pragma once

#include <pybind11/pybind11.h>

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>

#include <simplnx/Common/ScopeGuard.hpp>
#include <simplnx/Core/Application.hpp>
#include <simplnx/Filter/Arguments.hpp>
#include <simplnx/Filter/FilterTraits.hpp>
#include <simplnx/Filter/IFilter.hpp>
#include <simplnx/Filter/IParameter.hpp>
#include <simplnx/Filter/ParameterTraits.hpp>
#include <simplnx/Filter/Parameters.hpp>
#include <simplnx/Plugin/AbstractPlugin.hpp>
#include <simplnx/Plugin/PluginLoader.hpp>

#include <any>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#define SIMPLNX_PY_BIND_CLASS(scope, className) pybind11::class_<className>(scope, #className)
#define SIMPLNX_PY_BIND_CLASS_VARIADIC(scope, className, ...) pybind11::class_<className, __VA_ARGS__>(scope, #className)
#define SIMPLNX_PY_BIND_PARAMETER(scope, className) SIMPLNX_PY_BIND_CLASS_VARIADIC(scope, className, nx::core::IParameter)

namespace nx::core::CxPybind
{
namespace py = pybind11;

template <class T>
using CastNameT = std::remove_cv_t<decltype(py::detail::make_caster<T>::name)>;

template <usize N, class... Ts>
boost::mp11::mp_list<Ts...> ExtractListFromDescr(py::detail::descr<N, Ts...>*)
{
  return boost::mp11::mp_list<Ts...>{};
}

template <class T>
using ParamTypeListT = decltype(ExtractListFromDescr(static_cast<CastNameT<T>*>(nullptr)));

inline std::string GetFullPythonNameFromType(py::handle type)
{
  return type.attr("__module__").cast<std::string>() + "." + type.attr("__qualname__").cast<std::string>();
}

inline std::string GetFullPythonNameFromObject(py::handle object)
{
  py::handle type = py::type::handle_of(object);
  if(!type)
  {
    return "";
  }
  return GetFullPythonNameFromType(type);
}

template <class T>
std::string GetRegisteredPythonTypeName()
{
  py::handle type = py::type::of<T>();
  if(!type)
  {
    return "";
  }
  return GetFullPythonNameFromType(type);
}

template <class T>
std::string GetFullPythonName();

template <usize N, class... Ts>
constexpr bool CheckIfPythonNameNeedsFixup(const py::detail::descr<N, Ts...>& descr) noexcept
{
  std::string_view text = descr.text;
  usize index = text.find("%");
  return index != std::string_view::npos;
}

template <class T>
std::string GetFixedPythonTypeCasterName()
{
  using namespace boost::mp11;

  std::string typeName = py::detail::make_caster<T>::name.text;
  using Types = ParamTypeListT<T>;
  static constexpr usize N = mp_size<Types>::value;
  mp_for_each<mp_iota_c<N>>([&](auto I) {
    using CurrentType = mp_at_c<Types, static_cast<usize>(I)>;
    usize index = typeName.find("%");
    if(index == std::string::npos)
    {
      return;
    }
    typeName.replace(index, 1, GetFullPythonName<CurrentType>());
  });

  return typeName;
}

template <class T>
std::string GetFullPythonName()
{
  // Use py::type::of for registered C++ types otherwise use py::detail::type_caster::name
  if constexpr(std::is_base_of_v<py::detail::type_caster_generic, py::detail::make_caster<T>>)
  {
    return GetRegisteredPythonTypeName<T>();
  }
  else
  {
    return GetFixedPythonTypeCasterName<T>();
  }
}

struct PyParameterInfo
{
  using ToPyObjectFunc = py::object (*)(const std::any&);
  using FromPyObjectFunc = std::any (*)(py::handle);

  ToPyObjectFunc toPyObjectFunc = {};
  FromPyObjectFunc fromPyObjectFunc = {};
  std::string pythonTypeName;

  template <class T>
  static PyParameterInfo Create()
  {
    std::string pythonTypeName = GetFullPythonName<T>();

    return PyParameterInfo{[](const std::any& value) -> py::object { return py::cast(std::any_cast<T>(value)); }, [](py::handle value) -> std::any { return std::make_any<T>(value.cast<T>()); },
                           std::move(pythonTypeName)};
  }
};

class PythonPlugin;

class Internals
{
public:
  static inline constexpr StringLiteral k_Key = "SIMPLNX_INTERNAL";

  Internals()
  : m_App(Application::GetOrCreateInstance())
  {
  }

  static Internals& Instance()
  {
    auto* ptr = py::get_shared_data(k_Key);
    if(ptr == nullptr)
    {
      throw std::runtime_error("Unable to acquire simplnx internals");
    }
    return *static_cast<Internals*>(ptr);
  }

  PyParameterInfo& at(const Uuid& uuid)
  {
    if(!contains(uuid))
    {
      throw std::runtime_error(fmt::format("simplnx has not registered the parameter '{}'", uuid.str()));
    }
    return m_ParameterConversionMap.at(uuid);
  }

  const PyParameterInfo& at(const Uuid& uuid) const
  {
    if(!contains(uuid))
    {
      throw std::runtime_error(fmt::format("simplnx has not registered the parameter '{}'", uuid.str()));
    }
    return m_ParameterConversionMap.at(uuid);
  }

  bool contains(const Uuid& uuid) const
  {
    return m_ParameterConversionMap.count(uuid) > 0;
  }

  template <class ParameterT>
  void addConversion()
  {
    static_assert(std::is_base_of_v<IParameter, ParameterT>);

    Uuid uuid = ParameterTraits<ParameterT>::uuid;
    if(m_ParameterConversionMap.count(uuid) > 0)
    {
      throw std::invalid_argument("Duplicate uuid");
    }
    m_ParameterConversionMap.insert({std::move(uuid), PyParameterInfo::Create<typename ParameterT::ValueType>()});
  }

  Application* getApp()
  {
    return m_App.get();
  }

  const Application* getApp() const
  {
    return m_App.get();
  }

  template <class T>
  const T* addPlugin(std::shared_ptr<T> plugin)
  {
    static_assert(std::is_base_of_v<AbstractPlugin, T>);

    const T* pluginPtr = plugin.get();
    FilterList* filterList = m_App->getFilterList();
    Uuid pluginUuid = plugin->getId();
    if(filterList->containsPlugin(pluginUuid))
    {
      pluginPtr = dynamic_cast<const T*>(filterList->getPlugin(pluginUuid));
    }
    else
    {
      auto pluginLoader = std::make_shared<InMemoryPluginLoader>(plugin);
      filterList->addPlugin(std::dynamic_pointer_cast<IPluginLoader>(pluginLoader));
    }

    return pluginPtr;
  }

  template <class T>
  const T* addPlugin()
  {
    return addPlugin(std::make_shared<T>());
  }

  void registerPluginPyFilters(const AbstractPlugin& plugin)
  {
    // Must be called after all types are registered
    std::vector<py::type> list;
    for(auto handle : plugin.getFilterHandles())
    {
      py::object filter = py::cast(plugin.createFilter(handle.getFilterId()));
      py::type filterType = py::type::of(filter);
      list.push_back(filterType);
    }

    m_PluginFilterMap.insert({plugin.getId(), list});
  }

  const std::vector<py::type>& getPluginPyFilters(const Uuid& pluginUuid)
  {
    return m_PluginFilterMap.at(pluginUuid);
  }

  void loadPythonPlugin(py::module_& mod);

  PythonPlugin* getPythonPlugin(const Uuid& id)
  {
    return m_PythonPlugins.at(id).get();
  }

  const PythonPlugin* getPythonPlugin(const Uuid& id) const
  {
    return m_PythonPlugins.at(id).get();
  }

  void reloadPythonPlugins();

  void unloadPythonPlugins();

  std::vector<std::shared_ptr<PythonPlugin>> getPythonPlugins()
  {
    std::vector<std::shared_ptr<PythonPlugin>> plugins;
    for(auto&& [id, plugin] : m_PythonPlugins)
    {
      plugins.push_back(plugin);
    }
    return plugins;
  }

private:
  std::unordered_map<Uuid, PyParameterInfo> m_ParameterConversionMap;
  std::unordered_map<Uuid, std::vector<py::type>> m_PluginFilterMap;
  std::unordered_map<Uuid, std::shared_ptr<PythonPlugin>> m_PythonPlugins;
  std::shared_ptr<Application> m_App;
};

inline py::dict ConvertArgsToDict(const Internals& internals, const Parameters& parameters, const Arguments& args)
{
  py::dict dict;

  for(const auto& [name, value] : args)
  {
    if(!parameters.contains(name))
    {
      continue;
    }

    const AnyParameter& anyParam = parameters.at(name);
    const IParameter& param = anyParam.getRef();
    Uuid uuid = param.uuid();
    py::object object = internals.at(uuid).toPyObjectFunc(value);
    dict[name.c_str()] = std::move(object);
  }
  return dict;
}

inline Arguments ConvertDictToArgs(const Internals& internals, const Parameters& parameters, const py::dict& dict)
{
  Arguments args;

  for(auto item : dict)
  {
    auto name = item.first.cast<std::string>();
    if(!parameters.contains(name))
    {
      continue;
    }

    const AnyParameter& anyParam = parameters.at(name);
    const IParameter& param = anyParam.getRef();
    Uuid uuid = param.uuid();
    std::any value = internals.at(uuid).fromPyObjectFunc(item.second);
    args.insert(std::move(name), std::move(value));
  }
  return args;
}

inline std::string MakePythonParamString(std::string_view variableName, std::string_view typeName)
{
  return fmt::format("{}: {}", variableName, typeName);
}

inline std::string MakePythonParamStringWithDefaulValue(std::string_view variableName, std::string_view typeName, std::string_view defaultValue)
{
  return fmt::format("{}: {} = {}", variableName, typeName, defaultValue);
}

template <class T>
std::string MakePythonParamString(const char* variableName)
{
  std::string typeName = GetFullPythonName<T>();
  return MakePythonParamString(variableName, typeName);
}

inline std::vector<std::string> MakePythonParamListString(const Parameters& params, const Internals& internals)
{
  std::vector<std::string> paramList;
  paramList.reserve(params.size() + 2);
  paramList.push_back(MakePythonParamString("data_structure", GetFullPythonName<DataStructure>()));
  // Indicates that the following parameters are keyword only
  // paramList.push_back("*");
  for(const auto& [name, p] : params)
  {
    const auto& info = internals.at(p->uuid());
    py::object defaultValue = info.toPyObjectFunc(p->defaultValue());
    std::string defaultValueString = py::repr(defaultValue).cast<std::string>();
    paramList.push_back(MakePythonParamStringWithDefaulValue(p->name(), info.pythonTypeName, defaultValueString));
  }
  return paramList;
}

template <class FilterT>
std::string MakePythonSignature(std::string_view funcName, const Internals& internals)
{
  FilterT f;

  std::vector<std::string> paramList = MakePythonParamListString(f.parameters(), internals);

  std::string returnTypeName = GetFullPythonName<IFilter::ExecuteResult>();

  std::string signature = fmt::format("{}({}) -> {}", funcName, fmt::join(paramList, ", "), returnTypeName);

  return signature;
}

inline void PyPrintMessage(const IFilter::Message& message)
{
  py::gil_scoped_acquire acquireGIL{};
  try
  {
    py::print(fmt::format("{}", message.message));
  } catch(const py::error_already_set& pyException)
  {
    // Can fail when called in a program without stdout (i.e. GUI on Windows)
    // Catch the exception to prevent the filter execution from failing
    // Should be replaced with the ability to pass in a message handler to the python execute
  }
}

inline IFilter::MessageHandler CreatePyMessageHandler()
{
  return IFilter::MessageHandler{&PyPrintMessage};
}

template <class FilterT>
auto BindFilter(py::handle scope, const Internals& internals)
{
  using namespace pybind11::literals;

  py::class_<FilterT, IFilter> filter(scope, FilterTraits<FilterT>::className.c_str());
  filter.def(py::init<>());

  {
    py::options options;
    options.disable_function_signatures();

    std::string executeSig = MakePythonSignature<FilterT>("execute", internals);
    std::string executeDocString = fmt::format("{}\n\nExecutes the filter\n", executeSig);
    filter.def_static("human_name", [&internals]() {
      FilterT filter;
      return filter.humanName();
    });
    filter.def_static("name", [&internals]() {
      FilterT filter;
      return filter.name();
    });
    filter.def_static("uuid", [&internals]() {
      FilterT filter;
      return filter.uuid();
    });
    filter.def_static(
        "execute",
        [&internals](DataStructure& dataStructure, const py::kwargs& kwargs) {
          FilterT filter;

          Parameters parameters = filter.parameters();

          for(auto item : kwargs)
          {
            auto name = item.first.cast<std::string>();
            if(!parameters.contains(name))
            {
              throw py::type_error(fmt::format("execute() got an unexpected keyword argument '{}'", name));
            }
          }

          Arguments convertedArgs = ConvertDictToArgs(internals, filter.parameters(), kwargs);
          py::gil_scoped_release releaseGIL{};
          IFilter::ExecuteResult result = filter.execute(dataStructure, convertedArgs, nullptr, CreatePyMessageHandler());
          return result;
        },
        "data_structure"_a, executeDocString.c_str());

    // std::string preflightSig = MakePythonSignature<FilterT>("preflight", internals);
    // std::string preflightDocString = fmt::format("{}\n\nExecutes the filter\n", sig);

    // filter.def_static(
    //     "preflight",
    //     [&internals](DataStructure& dataStructure, const py::kwargs& kwargs) {
    //       FilterT filter;

    //      Parameters parameters = filter.parameters();

    //      for(auto item : kwargs)
    //      {
    //        auto name = item.first.cast<std::string>();
    //        if(!parameters.contains(name))
    //        {
    //          throw py::type_error(fmt::format("execute2() got an unexpected keyword argument '{}'", name));
    //        }
    //      }

    //      Arguments convertedArgs = ConvertDictToArgs(internals, filter.parameters(), kwargs);
    //      IFilter::PreflightResult result = filter.preflight(dataStructure, convertedArgs, CreatePyMessageHandler());
    //      return result;
    //    },
    //    "data_structure"_a, preflightDocString.c_str());
  }

  return filter;
}

template <class FilterT>
std::map<std::string, py::object> CreateDefaultArgs(const Internals& internals)
{
  FilterT filter;
  Parameters params = filter.parameters();
  std::map<std::string, py::object> defaults;
  for(const auto& [name, param] : params)
  {
    defaults.insert({name, internals.at(param->uuid()).toPyObjectFunc(param->defaultValue())});
  }

  return defaults;
}

class AtomicBoolProxy
{
public:
  AtomicBoolProxy() = delete;

  AtomicBoolProxy(const std::atomic_bool& atomicBool)
  : m_Value(&atomicBool)
  {
  }

  const std::atomic_bool* get() const
  {
    if(!isValid())
    {
      throw std::runtime_error("AtomicBoolProxy is invalid");
    }
    return m_Value;
  }

  bool isValid() const noexcept
  {
    return m_Value != nullptr;
  }

  void reset() noexcept
  {
    m_Value = nullptr;
  }

private:
  const std::atomic_bool* m_Value = nullptr;
};

inline auto MakeAtomicBoolProxyGuard(std::shared_ptr<AtomicBoolProxy>& proxy)
{
  return MakeScopeGuard([&proxy]() noexcept { proxy->reset(); });
}

class PyFilter : public IFilter
{
public:
  PyFilter() = delete;

  PyFilter(py::object object)
  : IFilter()
  , m_Object(std::move(object))
  {
    py::gil_scoped_acquire gil;
    m_Name = m_Object.attr("name")().cast<std::string>();
    m_ClassName = m_Object.attr("class_name")().cast<std::string>();
    m_Uuid = m_Object.attr("uuid")().cast<Uuid>();
    m_HumanName = m_Object.attr("human_name")().cast<std::string>();
    m_DefaultTags = m_Object.attr("default_tags")().cast<std::vector<std::string>>();
    m_Parameters = m_Object.attr("parameters")().cast<Parameters>();
  }

  ~PyFilter() noexcept
  {
    py::gil_scoped_acquire gil;
    m_Object = {};
  }

  PyFilter(const PyFilter&) = delete;
  PyFilter(PyFilter&&) noexcept = delete;

  PyFilter& operator=(const PyFilter&) = delete;
  PyFilter& operator=(PyFilter&&) noexcept = delete;

  std::string name() const override
  {
    return m_Name;
  }

  std::string className() const override
  {
    return m_ClassName;
  }

  Uuid uuid() const override
  {
    return m_Uuid;
  }

  std::string humanName() const override
  {
    return m_HumanName;
  }

  std::vector<std::string> defaultTags() const override
  {
    return m_DefaultTags;
  }

  Parameters parameters() const override
  {
    return m_Parameters;
  }

  UniquePointer clone() const override
  {
    py::gil_scoped_acquire gil;
    return std::make_unique<PyFilter>(m_Object.attr("clone")());
  }

protected:
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override
  {
    try
    {
      py::gil_scoped_acquire gil;
      Parameters params = parameters();
      auto shouldCancelProxy = std::make_shared<AtomicBoolProxy>(shouldCancel);
      auto guard = MakeAtomicBoolProxyGuard(shouldCancelProxy);
      auto result = m_Object.attr("preflight_impl")(py::cast(data, py::return_value_policy::reference), ConvertArgsToDict(Internals::Instance(), params, args), messageHandler, shouldCancelProxy)
                        .cast<PreflightResult>();
      return result;
    } catch(const py::error_already_set& pyException)
    {
      return {MakeErrorResult<OutputActions>(-42000, fmt::format("Python exception: {}", pyException.what()))};
    } catch(const std::exception& exception)
    {
      return {MakeErrorResult<OutputActions>(-42001, fmt::format("C++ exception: {}", exception.what()))};
    }
  }

  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override
  {
    try
    {
      py::gil_scoped_acquire gil;
      Parameters params = parameters();
      auto shouldCancelProxy = std::make_shared<AtomicBoolProxy>(shouldCancel);
      auto guard = MakeAtomicBoolProxyGuard(shouldCancelProxy);
      auto result =
          m_Object
              .attr("execute_impl")(py::cast(data, py::return_value_policy::reference), ConvertArgsToDict(Internals::Instance(), params, args), /* pipelineNode,*/ messageHandler, shouldCancelProxy)
              .cast<Result<>>();
      return result;
    } catch(const py::error_already_set& pyException)
    {
      return {MakeErrorResult(-42002, fmt::format("Python exception: {}", pyException.what()))};
    } catch(const std::exception& exception)
    {
      return {MakeErrorResult(-42003, fmt::format("C++ exception: {}", exception.what()))};
    }
  }

private:
  py::object m_Object;
  std::string m_Name;
  std::string m_ClassName;
  Uuid m_Uuid{};
  std::string m_HumanName;
  std::vector<std::string> m_DefaultTags;
  Parameters m_Parameters;
};

class PythonPlugin : public AbstractPlugin
{
public:
  ~PythonPlugin() noexcept override = default;

  static std::shared_ptr<PythonPlugin> Create(py::module_& mod)
  {
    py::gil_scoped_acquire gil;

    py::object object = mod.attr("get_plugin")();

    auto id = object.attr("id")().cast<Uuid>();
    auto name = object.attr("name")().cast<std::string>();
    auto description = object.attr("description")().cast<std::string>();
    auto vendor = object.attr("vendor")().cast<std::string>();
    auto filters = object.attr("get_filters")().cast<py::list>();

    auto plugin = std::shared_ptr<PythonPlugin>(new PythonPlugin(id, name, description, vendor));

    plugin->m_PythonModule = mod;

    for(py::handle filterTypeHandle : filters)
    {
      FilterCreationFunc filterCreationFunc = [filterType = py::reinterpret_borrow<py::object>(filterTypeHandle)]() -> IFilter::UniquePointer {
        py::gil_scoped_acquire gil;
        return std::make_unique<PyFilter>(filterType());
      };
      plugin->addFilter(std::move(filterCreationFunc));
    }

    return plugin;
  }

  SIMPLMapType getSimplToSimplnxMap() const override
  {
    return {};
  }

  void reload()
  {
    py::gil_scoped_acquire gil;
    m_PythonModule.reload();
  }

protected:
  PythonPlugin(IdType id, const std::string& name, const std::string& description, const std::string& vendor)
  : AbstractPlugin(id, name, description, vendor)
  {
  }

private:
  py::module_ m_PythonModule;
};

inline void Internals::loadPythonPlugin(py::module_& mod)
{
  auto plugin = PythonPlugin::Create(mod);

  auto pluginLoader = std::make_shared<InMemoryPluginLoader>(plugin);

  m_App->getFilterList()->addPlugin(std::dynamic_pointer_cast<IPluginLoader>(pluginLoader));

  m_PythonPlugins.insert({plugin->getId(), plugin});
}

inline void Internals::reloadPythonPlugins()
{
  FilterList* filterList = m_App->getFilterList();
  for(auto&& [id, plugin] : m_PythonPlugins)
  {
    filterList->removePlugin(id);
    plugin->reload();
    auto pluginLoader = std::make_shared<InMemoryPluginLoader>(plugin);
    filterList->addPlugin(std::dynamic_pointer_cast<IPluginLoader>(pluginLoader));
  }
}

inline void Internals::unloadPythonPlugins()
{
  FilterList* filterList = m_App->getFilterList();
  for(auto&& [id, plugin] : m_PythonPlugins)
  {
    filterList->removePlugin(id);
  }
  m_PythonPlugins.clear();
}

/**
 * @brief Convenience function for binding parameter constructors that folow the standard signature:
 * const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue
 * @tparam T
 * @tparam ...Options
 * @param object
 */
template <class T, class... Options>
void BindParameterConstructor(py::class_<T, Options...>& object)
{
  using namespace pybind11::literals;

  object.def(py::init<const std::string&, const std::string&, const std::string&, const typename T::ValueType&>(), "name"_a, "human_name"_a, "help_text"_a, "default_value"_a);
}
} // namespace nx::core::CxPybind
