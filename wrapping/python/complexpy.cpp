#include <pybind11/pybind11.h>

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include <ComplexCore/ComplexCorePlugin.hpp>
#include <ComplexCore/Filters/AbaqusHexahedronWriterFilter.hpp>
#include <ComplexCore/Filters/AddBadDataFilter.hpp>
#include <ComplexCore/Filters/AlignGeometries.hpp>
#include <ComplexCore/Filters/AlignSectionsFeatureCentroidFilter.hpp>
#include <ComplexCore/Filters/AlignSectionsListFilter.hpp>
#include <ComplexCore/Filters/AppendImageGeometryZSliceFilter.hpp>
#include <ComplexCore/Filters/ApplyTransformationToGeometryFilter.hpp>
#include <ComplexCore/Filters/ApproximatePointCloudHull.hpp>
#include <ComplexCore/Filters/ArrayCalculatorFilter.hpp>
#include <ComplexCore/Filters/AvizoRectilinearCoordinateWriterFilter.hpp>
#include <ComplexCore/Filters/AvizoUniformCoordinateWriterFilter.hpp>
#include <ComplexCore/Filters/CalculateArrayHistogramFilter.hpp>
#include <ComplexCore/Filters/CalculateFeatureSizesFilter.hpp>
#include <ComplexCore/Filters/CalculateTriangleAreasFilter.hpp>
#include <ComplexCore/Filters/ChangeAngleRepresentation.hpp>
#include <ComplexCore/Filters/CombineAttributeArraysFilter.hpp>
#include <ComplexCore/Filters/CombineStlFilesFilter.hpp>
#include <ComplexCore/Filters/ComputeFeatureRectFilter.hpp>
#include <ComplexCore/Filters/ComputeMomentInvariants2DFilter.hpp>
#include <ComplexCore/Filters/ConditionalSetValue.hpp>
#include <ComplexCore/Filters/ConvertColorToGrayScaleFilter.hpp>
#include <ComplexCore/Filters/ConvertDataFilter.hpp>
#include <ComplexCore/Filters/CopyDataObjectFilter.hpp>
#include <ComplexCore/Filters/CopyFeatureArrayToElementArray.hpp>
#include <ComplexCore/Filters/CreateAttributeMatrixFilter.hpp>
#include <ComplexCore/Filters/CreateDataArray.hpp>
#include <ComplexCore/Filters/CreateDataGroup.hpp>
#include <ComplexCore/Filters/CreateFeatureArrayFromElementArray.hpp>
#include <ComplexCore/Filters/CreateGeometryFilter.hpp>
#include <ComplexCore/Filters/CreateImageGeometry.hpp>
#include <ComplexCore/Filters/CropImageGeometry.hpp>
#include <ComplexCore/Filters/CropVertexGeometry.hpp>
#include <ComplexCore/Filters/DeleteData.hpp>
#include <ComplexCore/Filters/ErodeDilateBadDataFilter.hpp>
#include <ComplexCore/Filters/ErodeDilateCoordinationNumberFilter.hpp>
#include <ComplexCore/Filters/ErodeDilateMaskFilter.hpp>
#include <ComplexCore/Filters/ExecuteProcessFilter.hpp>
#include <ComplexCore/Filters/ExportDREAM3DFilter.hpp>
#include <ComplexCore/Filters/ExtractComponentAsArrayFilter.hpp>
#include <ComplexCore/Filters/ExtractInternalSurfacesFromTriangleGeometry.hpp>
#include <ComplexCore/Filters/ExtractVertexGeometryFilter.hpp>
#include <ComplexCore/Filters/FeatureDataCSVWriterFilter.hpp>
#include <ComplexCore/Filters/FillBadDataFilter.hpp>
#include <ComplexCore/Filters/FindArrayStatisticsFilter.hpp>
#include <ComplexCore/Filters/FindBiasedFeaturesFilter.hpp>
#include <ComplexCore/Filters/FindBoundaryCellsFilter.hpp>
#include <ComplexCore/Filters/FindBoundaryElementFractionsFilter.hpp>
#include <ComplexCore/Filters/FindDifferencesMap.hpp>
#include <ComplexCore/Filters/FindEuclideanDistMapFilter.hpp>
#include <ComplexCore/Filters/FindFeatureCentroidsFilter.hpp>
#include <ComplexCore/Filters/FindFeatureClusteringFilter.hpp>
#include <ComplexCore/Filters/FindFeaturePhasesBinaryFilter.hpp>
#include <ComplexCore/Filters/FindFeaturePhasesFilter.hpp>
#include <ComplexCore/Filters/FindLargestCrossSectionsFilter.hpp>
#include <ComplexCore/Filters/FindNeighborListStatistics.hpp>
#include <ComplexCore/Filters/FindNeighborhoodsFilter.hpp>
#include <ComplexCore/Filters/FindNeighbors.hpp>
#include <ComplexCore/Filters/FindNumFeaturesFilter.hpp>
#include <ComplexCore/Filters/FindSurfaceAreaToVolumeFilter.hpp>
#include <ComplexCore/Filters/FindSurfaceFeatures.hpp>
#include <ComplexCore/Filters/FindTriangleGeomCentroidsFilter.hpp>
#include <ComplexCore/Filters/FindTriangleGeomSizesFilter.hpp>
#include <ComplexCore/Filters/FindVertexToTriangleDistancesFilter.hpp>
#include <ComplexCore/Filters/FindVolFractionsFilter.hpp>
#include <ComplexCore/Filters/GenerateColorTableFilter.hpp>
#include <ComplexCore/Filters/IdentifySample.hpp>
#include <ComplexCore/Filters/ImageContouringFilter.hpp>
#include <ComplexCore/Filters/ImportBinaryCTNorthstarFilter.hpp>
#include <ComplexCore/Filters/ImportCSVDataFilter.hpp>
#include <ComplexCore/Filters/ImportDREAM3DFilter.hpp>
#include <ComplexCore/Filters/ImportHDF5Dataset.hpp>
#include <ComplexCore/Filters/ImportTextFilter.hpp>
#include <ComplexCore/Filters/ImportVolumeGraphicsFileFilter.hpp>
#include <ComplexCore/Filters/InitializeData.hpp>
#include <ComplexCore/Filters/InterpolatePointCloudToRegularGridFilter.hpp>
#include <ComplexCore/Filters/IterativeClosestPointFilter.hpp>
#include <ComplexCore/Filters/LaplacianSmoothingFilter.hpp>
#include <ComplexCore/Filters/MapPointCloudToRegularGridFilter.hpp>
#include <ComplexCore/Filters/MinNeighbors.hpp>
#include <ComplexCore/Filters/MoveData.hpp>
#include <ComplexCore/Filters/MultiThresholdObjects.hpp>
#include <ComplexCore/Filters/NearestPointFuseRegularGridsFilter.hpp>
#include <ComplexCore/Filters/PartitionGeometryFilter.hpp>
#include <ComplexCore/Filters/PointSampleTriangleGeometryFilter.hpp>
#include <ComplexCore/Filters/QuickSurfaceMeshFilter.hpp>
#include <ComplexCore/Filters/RawBinaryReaderFilter.hpp>
#include <ComplexCore/Filters/RegularGridSampleSurfaceMeshFilter.hpp>
#include <ComplexCore/Filters/RemoveFlaggedFeaturesFilter.hpp>
#include <ComplexCore/Filters/RemoveFlaggedVertices.hpp>
#include <ComplexCore/Filters/RemoveMinimumSizeFeaturesFilter.hpp>
#include <ComplexCore/Filters/RenameDataObject.hpp>
#include <ComplexCore/Filters/ReplaceElementAttributesWithNeighborValuesFilter.hpp>
#include <ComplexCore/Filters/ResampleImageGeomFilter.hpp>
#include <ComplexCore/Filters/ResampleRectGridToImageGeomFilter.hpp>
#include <ComplexCore/Filters/RobustAutomaticThreshold.hpp>
#include <ComplexCore/Filters/RotateSampleRefFrameFilter.hpp>
#include <ComplexCore/Filters/ScalarSegmentFeaturesFilter.hpp>
#include <ComplexCore/Filters/SetImageGeomOriginScalingFilter.hpp>
#include <ComplexCore/Filters/SharedFeatureFaceFilter.hpp>
#include <ComplexCore/Filters/SplitAttributeArrayFilter.hpp>
#include <ComplexCore/Filters/StlFileReaderFilter.hpp>
#include <ComplexCore/Filters/TriangleCentroidFilter.hpp>
#include <ComplexCore/Filters/TriangleDihedralAngleFilter.hpp>
#include <ComplexCore/Filters/TriangleNormalFilter.hpp>
#include <ComplexCore/Filters/UncertainRegularGridSampleSurfaceMeshFilter.hpp>
#include <ComplexCore/Filters/VtkRectilinearGridWriterFilter.hpp>
#include <ComplexCore/Filters/WriteASCIIDataFilter.hpp>
#include <ComplexCore/Filters/WriteBinaryDataFilter.hpp>
#include <ComplexCore/Filters/WriteStlFileFilter.hpp>

#include <complex/Common/ScopeGuard.hpp>
#include <complex/Core/Application.hpp>
#include <complex/DataStructure/AttributeMatrix.hpp>
#include <complex/DataStructure/DataArray.hpp>
#include <complex/DataStructure/DataGroup.hpp>
#include <complex/DataStructure/DataStore.hpp>
#include <complex/DataStructure/DataStructure.hpp>
#include <complex/DataStructure/Geometry/EdgeGeom.hpp>
#include <complex/DataStructure/Geometry/HexahedralGeom.hpp>
#include <complex/DataStructure/Geometry/IGeometry.hpp>
#include <complex/DataStructure/Geometry/ImageGeom.hpp>
#include <complex/DataStructure/Geometry/QuadGeom.hpp>
#include <complex/DataStructure/Geometry/RectGridGeom.hpp>
#include <complex/DataStructure/Geometry/TetrahedralGeom.hpp>
#include <complex/DataStructure/Geometry/TriangleGeom.hpp>
#include <complex/DataStructure/Geometry/VertexGeom.hpp>
#include <complex/Filter/Actions/CopyArrayInstanceAction.hpp>
#include <complex/Filter/Actions/CopyDataObjectAction.hpp>
#include <complex/Filter/Actions/CreateArrayAction.hpp>
#include <complex/Filter/Actions/CreateAttributeMatrixAction.hpp>
#include <complex/Filter/Actions/CreateDataGroupAction.hpp>
#include <complex/Filter/Actions/CreateGeometry1DAction.hpp>
#include <complex/Filter/Actions/CreateGeometry2DAction.hpp>
#include <complex/Filter/Actions/CreateGeometry3DAction.hpp>
#include <complex/Filter/Actions/CreateImageGeometryAction.hpp>
#include <complex/Filter/Actions/CreateNeighborListAction.hpp>
#include <complex/Filter/Actions/CreateRectGridGeometryAction.hpp>
#include <complex/Filter/Actions/CreateStringArrayAction.hpp>
#include <complex/Filter/Actions/CreateVertexGeometryAction.hpp>
#include <complex/Filter/Actions/DeleteDataAction.hpp>
#include <complex/Filter/Actions/EmptyAction.hpp>
#include <complex/Filter/Actions/ImportH5ObjectPathsAction.hpp>
#include <complex/Filter/Actions/ImportObjectAction.hpp>
#include <complex/Filter/Actions/MoveDataAction.hpp>
#include <complex/Filter/Actions/RenameDataAction.hpp>
#include <complex/Filter/Actions/UpdateImageGeomAction.hpp>
#include <complex/Filter/IFilter.hpp>
#include <complex/Filter/IParameter.hpp>
#include <complex/Filter/Parameters.hpp>
#include <complex/Parameters/ArrayCreationParameter.hpp>
#include <complex/Parameters/ArraySelectionParameter.hpp>
#include <complex/Parameters/ArrayThresholdsParameter.hpp>
#include <complex/Parameters/AttributeMatrixSelectionParameter.hpp>
#include <complex/Parameters/BoolParameter.hpp>
#include <complex/Parameters/CalculatorParameter.hpp>
#include <complex/Parameters/ChoicesParameter.hpp>
#include <complex/Parameters/DataGroupCreationParameter.hpp>
#include <complex/Parameters/DataGroupSelectionParameter.hpp>
#include <complex/Parameters/DataObjectNameParameter.hpp>
#include <complex/Parameters/DataPathSelectionParameter.hpp>
#include <complex/Parameters/DataStoreFormatParameter.hpp>
#include <complex/Parameters/DataTypeParameter.hpp>
#include <complex/Parameters/Dream3dImportParameter.hpp>
#include <complex/Parameters/DynamicTableParameter.hpp>
#include <complex/Parameters/EnsembleInfoParameter.hpp>
#include <complex/Parameters/FileSystemPathParameter.hpp>
#include <complex/Parameters/GenerateColorTableParameter.hpp>
#include <complex/Parameters/GeneratedFileListParameter.hpp>
#include <complex/Parameters/GeometrySelectionParameter.hpp>
#include <complex/Parameters/ImportCSVDataParameter.hpp>
#include <complex/Parameters/ImportHDF5DatasetParameter.hpp>
#include <complex/Parameters/MultiArraySelectionParameter.hpp>
#include <complex/Parameters/MultiPathSelectionParameter.hpp>
#include <complex/Parameters/NeighborListSelectionParameter.hpp>
#include <complex/Parameters/NumberParameter.hpp>
#include <complex/Parameters/NumericTypeParameter.hpp>
#include <complex/Parameters/StringParameter.hpp>
#include <complex/Parameters/VectorParameter.hpp>
#include <complex/Pipeline/AbstractPipelineNode.hpp>
#include <complex/Pipeline/Pipeline.hpp>
#include <complex/Pipeline/PipelineFilter.hpp>

#include <fmt/ranges.h>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>

#define COMPLEX_PY_BIND_CLASS(scope, className) pybind11::class_<className>(scope, #className)
#define COMPLEX_PY_BIND_CLASS_VARIADIC(scope, className, ...) pybind11::class_<className, __VA_ARGS__>(scope, #className)
#define COMPLEX_PY_BIND_PARAMETER(scope, className) COMPLEX_PY_BIND_CLASS_VARIADIC(scope, className, complex::IParameter)

using namespace complex;
namespace py = pybind11;

using namespace pybind11::literals;

template <>
struct fmt::formatter<complex::Error>
{
  constexpr format_parse_context::iterator parse(format_parse_context& ctx)
  {
    return ctx.begin();
  }

  format_context::iterator format(const complex::Error& value, format_context& ctx) const
  {
    return fmt::format_to(ctx.out(), "Error(code={}, message='{}')", value.code, value.message);
  }
};

template <>
struct fmt::formatter<complex::Warning>
{
  constexpr format_parse_context::iterator parse(format_parse_context& ctx)
  {
    return ctx.begin();
  }

  format_context::iterator format(const complex::Warning& value, format_context& ctx) const
  {
    return fmt::format_to(ctx.out(), "Warning(code={}, message='{}')", value.code, value.message);
  }
};

template <class T>
using CastNameT = std::remove_cv_t<decltype(py::detail::make_caster<T>::name)>;

template <usize N, class... Ts>
boost::mp11::mp_list<Ts...> ExtractListFromDescr(py::detail::descr<N, Ts...>*)
{
  return boost::mp11::mp_list<Ts...>{};
}

template <class T>
using ParamTypeListT = decltype(ExtractListFromDescr(static_cast<CastNameT<T>*>(nullptr)));

std::string GetFullPythonNameFromType(py::handle type)
{
  return type.attr("__module__").cast<std::string>() + "." + type.attr("__qualname__").cast<std::string>();
}

std::string GetFullPythonNameFromObject(py::handle object)
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
  static inline constexpr StringLiteral k_Key = "COMPLEX_INTERNAL";

  Internals()
  : m_App(Application::GetOrCreateInstance())
  {
  }

  static Internals& Instance()
  {
    auto* ptr = py::get_shared_data(k_Key);
    if(ptr == nullptr)
    {
      throw std::runtime_error("Unable to acquire complex internals");
    }
    return *static_cast<Internals*>(ptr);
  }

  PyParameterInfo& at(const Uuid& uuid)
  {
    if(!contains(uuid))
    {
      throw std::runtime_error(fmt::format("complex has not registered the parameter '{}'", uuid.str()));
    }
    return m_ParameterConversionMap.at(uuid);
  }

  const PyParameterInfo& at(const Uuid& uuid) const
  {
    if(!contains(uuid))
    {
      throw std::runtime_error(fmt::format("complex has not registered the parameter '{}'", uuid.str()));
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
    return m_App;
  }

  const Application* getApp() const
  {
    return m_App;
  }

  template <class T>
  const T* addPlugin()
  {
    static_assert(std::is_base_of_v<AbstractPlugin, T>);

    auto plugin = std::make_shared<T>();
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
  Application* m_App;
};

py::dict ConvertArgsToDict(const Internals& internals, const Parameters& parameters, const Arguments& args)
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

Arguments ConvertDictToArgs(const Internals& internals, const Parameters& parameters, const py::dict& dict)
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

std::string MakePythonParamString(std::string_view variableName, std::string_view typeName)
{
  return fmt::format("{}: {}", variableName, typeName);
}

std::string MakePythonParamStringWithDefaulValue(std::string_view variableName, std::string_view typeName, std::string_view defaultValue)
{
  return fmt::format("{}: {} = {}", variableName, typeName, defaultValue);
}

template <class T>
std::string MakePythonParamString(const char* variableName)
{
  std::string typeName = GetFullPythonName<T>();
  return MakePythonParamString(variableName, typeName);
}

std::vector<std::string> MakePythonParamListString(const Parameters& params, const Internals& internals)
{
  std::vector<std::string> paramList;
  paramList.reserve(params.size() + 2);
  paramList.push_back(MakePythonParamString("data_structure", GetFullPythonName<DataStructure>()));
  // Indicates that the following parameters are keyword only
  paramList.push_back("*");
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

void PyPrintMessage(const IFilter::Message& message)
{
  py::print(fmt::format("{}", message.message));
}

IFilter::MessageHandler CreatePyMessageHandler()
{
  return IFilter::MessageHandler{&PyPrintMessage};
}

template <class FilterT>
auto BindFilter(py::handle scope, const Internals& internals)
{
  py::class_<FilterT, IFilter> filter(scope, FilterTraits<FilterT>::className.c_str());
  filter.def(py::init<>());

  {
    py::options options;
    options.disable_function_signatures();

    std::string executeSig = MakePythonSignature<FilterT>("execute", internals);
    std::string executeDocString = fmt::format("{}\n\nExecutes the filter\n", executeSig);

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
              throw py::type_error(fmt::format("execute2() got an unexpected keyword argument '{}'", name));
            }
          }

          Arguments convertedArgs = ConvertDictToArgs(internals, filter.parameters(), kwargs);
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

auto MakeAtomicBoolProxyGuard(std::shared_ptr<AtomicBoolProxy>& proxy)
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
      auto result = m_Object.attr("preflight_impl")(data, ConvertArgsToDict(Internals::Instance(), params, args), messageHandler, shouldCancelProxy).cast<PreflightResult>();
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
      auto result = m_Object.attr("execute_impl")(data, ConvertArgsToDict(Internals::Instance(), params, args), /* pipelineNode,*/ messageHandler, shouldCancelProxy).cast<Result<>>();
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

  std::map<complex::Uuid, complex::Uuid> getSimplToComplexMap() const override
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

void Internals::loadPythonPlugin(py::module_& mod)
{
  auto plugin = PythonPlugin::Create(mod);

  auto pluginLoader = std::make_shared<InMemoryPluginLoader>(plugin);

  m_App->getFilterList()->addPlugin(std::dynamic_pointer_cast<IPluginLoader>(pluginLoader));

  m_PythonPlugins.insert({plugin->getId(), plugin});
}

void Internals::reloadPythonPlugins()
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

template <class ParameterT>
void PyInsertLinkableParameter(Parameters& self, const ParameterT& param)
{
  auto clonedParam = std::unique_ptr<ParameterT>(dynamic_cast<ParameterT*>(param.clone().release()));
  self.insertLinkableParameter(std::move(clonedParam));
}

template <class ParameterT>
auto BindNumberParameter(py::handle scope, const char* name)
{
  auto numberParameter = py::class_<ParameterT, IParameter>(scope, name);
  numberParameter.def(py::init<const std::string&, const std::string&, const std::string&, typename ParameterT::ValueType>());
  return numberParameter;
}

template <class ParameterT>
auto BindVectorParameter(py::handle scope, const char* name)
{
  auto vectorParameter = py::class_<ParameterT, IParameter>(scope, name);
  vectorParameter.def(py::init<const std::string&, const std::string&, const std::string&, const typename ParameterT::ValueType&>());
  vectorParameter.def(py::init<const std::string&, const std::string&, const std::string&, const typename ParameterT::ValueType&, const typename ParameterT::NamesType&>());
  return vectorParameter;
}

#define COMPLEX_PY_BIND_NUMBER_PARAMETER(scope, className) BindNumberParameter<className>(scope, #className)
#define COMPLEX_PY_BIND_VECTOR_PARAMETER(scope, className) BindVectorParameter<className>(scope, #className)

template <class T>
auto BindDataStore(py::handle scope, const char* name)
{
  py::class_<DataStore<T>, AbstractDataStore<T>, std::shared_ptr<DataStore<T>>> dataStore(scope, name);
  dataStore.def(py::init<const IDataStore::ShapeType&, const IDataStore::ShapeType&, std::optional<T>>(), "tuple_shape"_a, "component_shape"_a, "init_value"_a = std::optional<T>{});
  dataStore.def_property_readonly_static("dtype", []([[maybe_unused]] py::object self) { return py::dtype::of<T>(); });
  dataStore.def(
      "npview",
      [](DataStore<T>& dataStore) {
        IDataStore::ShapeType shape = dataStore.getTupleShape();
        IDataStore::ShapeType componentShape = dataStore.getComponentShape();
        shape.insert(shape.end(), componentShape.cbegin(), componentShape.cend());
        return py::array_t<T, py::array::c_style>(shape, dataStore.data(), py::cast(dataStore));
      },
      py::return_value_policy::reference_internal);
  dataStore.def("__getitem__", &DataStore<T>::at);
  dataStore.def("__len__", &DataStore<T>::getSize);
  return dataStore;
}

template <class T>
auto BindDataArray(py::handle scope, const char* name)
{
  py::class_<DataArray<T>, IDataArray, std::shared_ptr<DataArray<T>>> dataArray(scope, name);
  dataArray.def_property_readonly_static("dtype", []([[maybe_unused]] py::object self) { return py::dtype::of<T>(); });
  return dataArray;
}

#define COMPLEX_PY_BIND_DATA_ARRAY(scope, className) BindDataArray<className::value_type>(scope, #className)
#define COMPLEX_PY_BIND_DATA_STORE(scope, className) BindDataStore<className::value_type>(scope, #className)
#define COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(scope, className) COMPLEX_PY_BIND_CLASS_VARIADIC(scope, className, IDataStore, std::shared_ptr<className>)

template <class GeomT>
auto BindCreateGeometry2DAction(py::handle scope, const char* name)
{
  auto createGeometry2DAction = py::class_<GeomT, IDataCreationAction>(scope, name);
  createGeometry2DAction.def(py::init<const DataPath&, size_t, size_t, const std::string&, const std::string&, const std::string&, const std::string&>(), "geometry_path"_a, "num_faces"_a,
                             "num_vertices"_a, "vertex_attribute_matrix_name"_a, "face_attribute_matrix_name"_a, "shared_vertices_name"_a, "shared_faces_name"_a);
  createGeometry2DAction.def(py::init<const DataPath&, const DataPath&, const DataPath&, const std::string&, const std::string&, const IDataCreationAction::ArrayHandlingType&>(), "geometry_path"_a,
                             "input_vertices_array_path"_a, "input_faces_array_path"_a, "vertex_attribute_matrix_name"_a, "face_attribute_matrix_name"_a, "array_type"_a);
  return createGeometry2DAction;
}

template <class GeomT>
auto BindCreateGeometry3DAction(py::handle scope, const char* name)
{
  auto createGeometry3DAction = py::class_<GeomT, IDataCreationAction>(scope, name);
  createGeometry3DAction.def(py::init<const DataPath&, size_t, size_t, const std::string&, const std::string&, const std::string&, const std::string&>(), "geometry_path"_a, "num_cells"_a,
                             "num_vertices"_a, "vertex_data_name"_a, "cell_data_name"_a, "shared_vertices_name"_a, "shared_cells_name"_a);
  createGeometry3DAction.def(py::init<const DataPath&, const DataPath&, const DataPath&, const std::string&, const std::string&, const IDataCreationAction::ArrayHandlingType&>(), "geometry_path"_a,
                             "input_vertices_array_path"_a, "input_cell_array_path"_a, "vertex_attribute_matrix_name"_a, "cell_attribute_matrix_name"_a, "array_type"_a);
  return createGeometry3DAction;
}

#define COMPLEX_PY_BIND_CREATE_GEOMETRY_2D_ACTION(scope, className) BindCreateGeometry2DAction<className>(scope, #className)
#define COMPLEX_PY_BIND_CREATE_GEOMETRY_3D_ACTION(scope, className) BindCreateGeometry3DAction<className>(scope, #className)

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
  object.def(py::init<const std::string&, const std::string&, const std::string&, const typename T::ValueType&>(), "name"_a, "human_name"_a, "help_text"_a, "default_value"_a);
}

PYBIND11_MODULE(complex, mod)
{
  auto* internals = new Internals();

  const auto* corePlugin = internals->addPlugin<ComplexCorePlugin>();

  py::set_shared_data(Internals::k_Key, internals);

  // This is required until the pybind11_json library is added which adds the appropriate typecasters
  auto json = py::class_<nlohmann::json>(mod, "Json");
  json.def(py::init<>([](std::string_view text) { return nlohmann::json::parse(text); }), "text"_a);
  json.def("__str__", [](nlohmann::json& self) { return self.dump(); });

  py::class_<Error> error(mod, "Error");
  error.def(py::init<>());
  error.def(py::init<int32, std::string>());
  error.def_readwrite("code", &Error::code);
  error.def_readwrite("message", &Error::message);
  error.def("__repr__", [](const Error& self) { return fmt::format("<complex.Error(code={}, message='{}')>", self.code, self.message); });
  error.def("__str__", [](const Error& self) { return fmt::format("<complex.Error(code={}, message='{}')>", self.code, self.message); });

  py::class_<Warning> warning(mod, "Warning");
  warning.def(py::init<>());
  warning.def(py::init<int32, std::string>());
  warning.def_readwrite("code", &Warning::code);
  warning.def_readwrite("message", &Warning::message);
  warning.def("__repr__", [](const Warning& self) { return fmt::format("<complex.Warning(code={}, message='{}')>", self.code, self.message); });
  warning.def("__str__", [](const Warning& self) { return fmt::format("<complex.Warning(code={}, message='{}')>", self.code, self.message); });

  py::class_<Result<>> result(mod, "Result");
  result.def(py::init<>([](std::optional<std::vector<Error>> errors, std::optional<std::vector<Warning>> warnings) {
               Result<> result;
               if(errors.has_value())
               {
                 result.m_Expected = nonstd::make_unexpected(std::move(*errors));
               }
               if(warnings.has_value())
               {
                 result.warnings() = std::move(*warnings);
               }
               return result;
             }),
             "errors"_a = py::none(), "warnings"_a = py::none());
  result.def_property(
      "errors", [](Result<>& self) { return self.errors(); }, [](Result<>& self, std::vector<Error> errors) { self.errors() = std::move(errors); });
  result.def_property(
      "warnings", [](Result<>& self) { return self.warnings(); }, [](Result<>& self, std::vector<Warning> warnings) { self.warnings() = std::move(warnings); });

  py::enum_<NumericType> numericType(mod, "NumericType");
  numericType.value("int8", NumericType::int8);
  numericType.value("uint8", NumericType::uint8);
  numericType.value("int16", NumericType::int16);
  numericType.value("uint16", NumericType::uint16);
  numericType.value("int32", NumericType::int32);
  numericType.value("uint32", NumericType::uint32);
  numericType.value("int64", NumericType::int64);
  numericType.value("uint64", NumericType::uint64);
  numericType.value("float32", NumericType::float32);
  numericType.value("float64", NumericType::float64);

  py::enum_<DataType> dataType(mod, "DataType");
  dataType.value("int8", DataType::int8);
  dataType.value("uint8", DataType::uint8);
  dataType.value("int16", DataType::int16);
  dataType.value("uint16", DataType::uint16);
  dataType.value("int32", DataType::int32);
  dataType.value("uint32", DataType::uint32);
  dataType.value("int64", DataType::int64);
  dataType.value("uint64", DataType::uint64);
  dataType.value("float32", DataType::float32);
  dataType.value("float64", DataType::float64);
  dataType.value("boolean", DataType::boolean);

  py::class_<Uuid> uuid(mod, "Uuid");
  uuid.def(py::init<>());
  uuid.def(py::init([](std::string_view text) {
    std::optional<Uuid> uuid = Uuid::FromString(text);
    if(!uuid.has_value())
    {
      throw std::invalid_argument(fmt::format("Invalid uuid string '{}'", text));
    }
    return *uuid;
  }));
  uuid.def("__str__", &Uuid::str);
  uuid.def("__repr__", [](const Uuid& self) { return fmt::format("<complex.Uuid('{}')>", self.str()); });
  uuid.def("__getitem__", [](const Uuid& self, usize i) { return static_cast<uint8>(self.data.at(i)); });
  uuid.def("__setitem__", [](Uuid& self, usize i, uint8 value) { self.data.at(i) = std::byte{value}; });
  uuid.def("__len__", [](const Uuid& self) { return self.data.size(); });
  uuid.def_property_readonly("bytes", [](const Uuid& self) { return py::bytes(reinterpret_cast<const char*>(self.data.data()), self.data.size()); });

  py::class_<AtomicBoolProxy, std::shared_ptr<AtomicBoolProxy>> atomicBool(mod, "AtomicBoolProxy");
  atomicBool.def("__bool__", [](const AtomicBoolProxy& self) { return self.get()->load(); });

  py::class_<DataPath> dataPath(mod, "DataPath");
  dataPath.def(py::init<>());
  dataPath.def(py::init<std::vector<std::string>>());
  dataPath.def("__getitem__", [](const DataPath& self, usize index) { return self[index]; });
  dataPath.def("__repr__", [](const DataPath& self) { return fmt::format("DataPath('{}')", self.toString("/")); });
  dataPath.def("__str__", [](const DataPath& self) { return fmt::format("DataPath('{}')", self.toString("/")); });

  py::class_<AbstractPipelineNode, std::shared_ptr<AbstractPipelineNode>> abstractPipelineNode(mod, "AbstractPipelineNode");
  py::class_<PipelineFilter, AbstractPipelineNode, std::shared_ptr<PipelineFilter>> pipelineFilter(mod, "PipelineFilter");

  py::class_<IParameter> parameter(mod, "IParameter");

  py::enum_<IParameter::Type> parameterType(parameter, "Type");
  parameterType.value("Value", IParameter::Type::Value);
  parameterType.value("Data", IParameter::Type::Data);

  parameter.def_property_readonly("name", &IParameter::name);
  parameter.def_property_readonly("uuid", &IParameter::uuid);
  parameter.def_property_readonly("human_name", &IParameter::humanName);
  parameter.def_property_readonly("help_text", &IParameter::helpText);
  parameter.def_property_readonly("type", &IParameter::type);

  py::class_<Parameters> parameters(mod, "Parameters");

  py::class_<Parameters::Separator> separator(parameters, "Separator");
  separator.def(py::init<>());
  separator.def(py::init<std::string>(), "name"_a);
  separator.def_readwrite("name", &Parameters::Separator::name);

  parameters.def(py::init<>());
  parameters.def("insert", [](Parameters& self, const IParameter& param) { self.insert(param.clone()); });
  parameters.def("insert", py::overload_cast<Parameters::Separator>(&Parameters::insert));
  parameters.def("insert_linkable_parameter", &PyInsertLinkableParameter<BoolParameter>);
  parameters.def("insert_linkable_parameter", &PyInsertLinkableParameter<ChoicesParameter>);
  parameters.def("link_parameters", [](Parameters& self, std::string groupKey, std::string childKey, BoolParameter::ValueType value) { self.linkParameters(groupKey, childKey, value); });
  parameters.def("link_parameters", [](Parameters& self, std::string groupKey, std::string childKey, ChoicesParameter::ValueType value) { self.linkParameters(groupKey, childKey, value); });
  parameters.def(
      "__getitem__", [](Parameters& self, std::string_view key) { return self.at(key).get(); }, py::return_value_policy::reference_internal);

  py::class_<IArrayThreshold, std::shared_ptr<IArrayThreshold>> iArrayThreshold(mod, "IArrayThreshold");

  py::enum_<IArrayThreshold::UnionOperator> unionOperator(iArrayThreshold, "UnionOperator");
  unionOperator.value("And", IArrayThreshold::UnionOperator::And);
  unionOperator.value("Or", IArrayThreshold::UnionOperator::Or);

  iArrayThreshold.def_property("inverted", &IArrayThreshold::isInverted, &IArrayThreshold::setInverted);
  iArrayThreshold.def_property("union_op", &IArrayThreshold::getUnionOperator, &IArrayThreshold::setUnionOperator);
  iArrayThreshold.def("get_required_paths", &IArrayThreshold::getRequiredPaths);

  py::class_<ArrayThreshold, IArrayThreshold, std::shared_ptr<ArrayThreshold>> arrayThreshold(mod, "ArrayThreshold");

  py::enum_<ArrayThreshold::ComparisonType> comparisionType(arrayThreshold, "ComparisonType");
  comparisionType.value("GreaterThan", ArrayThreshold::ComparisonType::GreaterThan);
  comparisionType.value("LessThan", ArrayThreshold::ComparisonType::LessThan);
  comparisionType.value("Equal", ArrayThreshold::ComparisonType::Operator_Equal);
  comparisionType.value("NotEqual", ArrayThreshold::ComparisonType::Operator_NotEqual);

  arrayThreshold.def(py::init<>());
  arrayThreshold.def_property("array_path", &ArrayThreshold::getArrayPath, &ArrayThreshold::setArrayPath);
  arrayThreshold.def_property("value", &ArrayThreshold::getComparisonValue, &ArrayThreshold::setComparisonValue);
  arrayThreshold.def_property("comparison", &ArrayThreshold::getComparisonType, &ArrayThreshold::setComparisonType);

  py::class_<ArrayThresholdSet, IArrayThreshold, std::shared_ptr<ArrayThresholdSet>> arrayThresholdSet(mod, "ArrayThresholdSet");
  arrayThresholdSet.def(py::init<>());
  arrayThresholdSet.def_property("thresholds", &ArrayThresholdSet::getArrayThresholds, &ArrayThresholdSet::setArrayThresholds);
  arrayThresholdSet.def("__repr__", [](const ArrayThresholdSet& self) { return "ArrayThresholdSet()"; });

  py::class_<CSVWizardData> csvWizardData(mod, "CSVWizardData");

  py::enum_<CSVWizardData::HeaderMode> csvHeaderMode(csvWizardData, "HeaderMode");
  csvHeaderMode.value("Line", CSVWizardData::HeaderMode::LINE);
  csvHeaderMode.value("Custom", CSVWizardData::HeaderMode::CUSTOM);
  csvHeaderMode.value("Defaults", CSVWizardData::HeaderMode::DEFAULTS);

  csvWizardData.def(py::init<>());
  csvWizardData.def_readwrite("input_file_path", &CSVWizardData::inputFilePath);
  csvWizardData.def_readwrite("data_headers", &CSVWizardData::dataHeaders);
  csvWizardData.def_readwrite("begin_index", &CSVWizardData::beginIndex);
  csvWizardData.def_readwrite("number_of_lines", &CSVWizardData::numberOfLines);
  csvWizardData.def_readwrite("data_types", &CSVWizardData::dataTypes);
  csvWizardData.def_readwrite("delimiters", &CSVWizardData::delimiters);
  csvWizardData.def_readwrite("header_line", &CSVWizardData::headerLine);
  csvWizardData.def_readwrite("header_mode", &CSVWizardData::headerMode);
  csvWizardData.def_readwrite("tab_as_delimiter", &CSVWizardData::tabAsDelimiter);
  csvWizardData.def_readwrite("semicolon_as_delimiter", &CSVWizardData::semicolonAsDelimiter);
  csvWizardData.def_readwrite("comma_as_delimiter", &CSVWizardData::commaAsDelimiter);
  csvWizardData.def_readwrite("space_as_delimiter", &CSVWizardData::spaceAsDelimiter);
  csvWizardData.def_readwrite("consecutive_delimiters", &CSVWizardData::consecutiveDelimiters);
  csvWizardData.def("__repr__", [](const CSVWizardData& self) { return "CSVWizardData()"; });

  py::class_<AbstractPlugin, std::shared_ptr<AbstractPlugin>> abstractPlugin(mod, "AbstractPlugin");
  py::class_<PythonPlugin, AbstractPlugin, std::shared_ptr<PythonPlugin>> pythonPlugin(mod, "PythonPlugin");

  py::class_<IDataStore, std::shared_ptr<IDataStore>> iDataStore(mod, "IDataStore");
  iDataStore.def_property_readonly("data_type", &IDataStore::getDataType);
  iDataStore.def_property_readonly("tdims", &IDataStore::getTupleShape);
  iDataStore.def_property_readonly("cdims", &IDataStore::getComponentShape);

  auto abstractDataStoreInt8 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, Int8AbstractDataStore);
  auto abstractDataStoreUInt8 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, UInt8AbstractDataStore);
  auto abstractDataStoreInt16 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, Int16AbstractDataStore);
  auto abstractDataStoreUInt16 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, UInt16AbstractDataStore);
  auto abstractDataStoreInt32 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, Int32AbstractDataStore);
  auto abstractDataStoreUInt32 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, UInt32AbstractDataStore);
  auto abstractDataStoreInt64 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, Int64AbstractDataStore);
  auto abstractDataStoreUInt64 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, UInt64AbstractDataStore);
  auto abstractDataStoreFloat32 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, Float32AbstractDataStore);
  auto abstractDataStoreFloat64 = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, Float64AbstractDataStore);
  auto abstractDataStoreBool = COMPLEX_PY_BIND_ABSTRACT_DATA_STORE(mod, BoolAbstractDataStore);

  auto dataStoreInt8 = COMPLEX_PY_BIND_DATA_STORE(mod, Int8DataStore);
  auto dataStoreUInt8 = COMPLEX_PY_BIND_DATA_STORE(mod, UInt8DataStore);
  auto dataStoreInt16 = COMPLEX_PY_BIND_DATA_STORE(mod, Int16DataStore);
  auto dataStoreUInt16 = COMPLEX_PY_BIND_DATA_STORE(mod, UInt16DataStore);
  auto dataStoreInt32 = COMPLEX_PY_BIND_DATA_STORE(mod, Int32DataStore);
  auto dataStoreUInt32 = COMPLEX_PY_BIND_DATA_STORE(mod, UInt32DataStore);
  auto dataStoreInt64 = COMPLEX_PY_BIND_DATA_STORE(mod, Int64DataStore);
  auto dataStoreUInt64 = COMPLEX_PY_BIND_DATA_STORE(mod, UInt64DataStore);
  auto dataStoreFloat32 = COMPLEX_PY_BIND_DATA_STORE(mod, Float32DataStore);
  auto dataStoreFloat64 = COMPLEX_PY_BIND_DATA_STORE(mod, Float64DataStore);
  auto dataStoreBool = COMPLEX_PY_BIND_DATA_STORE(mod, BoolDataStore);

  py::class_<DataStructure> dataStructure(mod, "DataStructure");
  py::class_<DataObject, std::shared_ptr<DataObject>> dataObject(mod, "DataObject");

  dataObject.def_property_readonly("id", &DataObject::getId);
  dataObject.def_property_readonly("name", &DataObject::getName);

  dataStructure.def(py::init<>());
  dataStructure.def("__getitem__", py::overload_cast<const DataPath&>(&DataStructure::getSharedData));
  dataStructure.def_property_readonly("size", &DataStructure::getSize);
  dataStructure.def("__len__", &DataStructure::getSize);
  dataStructure.def("remove", py::overload_cast<const DataPath&>(&DataStructure::removeData));

  py::class_<BaseGroup, DataObject, std::shared_ptr<BaseGroup>> baseGroup(mod, "BaseGroup");
  baseGroup.def("contains", py::overload_cast<const std::string&>(&BaseGroup::contains, py::const_));
  baseGroup.def("__getitem__", py::overload_cast<const std::string&>(&BaseGroup::at), py::return_value_policy::reference_internal);
  baseGroup.def("__len__", &BaseGroup::getSize);

  auto baseGroupType = py::enum_<BaseGroup::GroupType>(baseGroup, "GroupType");
  baseGroupType.value("BaseGroup", BaseGroup::GroupType::BaseGroup);
  baseGroupType.value("DataGroup", BaseGroup::GroupType::DataGroup);
  baseGroupType.value("AttributeMatrix", BaseGroup::GroupType::AttributeMatrix);
  baseGroupType.value("IGeometry", BaseGroup::GroupType::IGeometry);
  baseGroupType.value("IGridGeometry", BaseGroup::GroupType::IGridGeometry);
  baseGroupType.value("RectGridGeom", BaseGroup::GroupType::RectGridGeom);
  baseGroupType.value("ImageGeom", BaseGroup::GroupType::ImageGeom);
  baseGroupType.value("INodeGeometry0D", BaseGroup::GroupType::INodeGeometry0D);
  baseGroupType.value("VertexGeom", BaseGroup::GroupType::VertexGeom);
  baseGroupType.value("INodeGeometry1D", BaseGroup::GroupType::INodeGeometry1D);
  baseGroupType.value("EdgeGeom", BaseGroup::GroupType::EdgeGeom);
  baseGroupType.value("INodeGeometry2D", BaseGroup::GroupType::INodeGeometry2D);
  baseGroupType.value("QuadGeom", BaseGroup::GroupType::QuadGeom);
  baseGroupType.value("TriangleGeom", BaseGroup::GroupType::TriangleGeom);
  baseGroupType.value("INodeGeometry3D", BaseGroup::GroupType::INodeGeometry3D);
  baseGroupType.value("HexahedralGeom", BaseGroup::GroupType::HexahedralGeom);
  baseGroupType.value("TetrahedralGeom", BaseGroup::GroupType::TetrahedralGeom);
  baseGroupType.value("Unknown", BaseGroup::GroupType::Unknown);

  py::class_<IGeometry, BaseGroup, std::shared_ptr<IGeometry>> iGeometry(mod, "IGeometry");

  py::enum_<IGeometry::Type> geomType(iGeometry, "Type");
  geomType.value("Image", IGeometry::Type::Image);
  geomType.value("RectGrid", IGeometry::Type::RectGrid);
  geomType.value("Vertex", IGeometry::Type::Vertex);
  geomType.value("Edge", IGeometry::Type::Edge);
  geomType.value("Triangle", IGeometry::Type::Triangle);
  geomType.value("Quad", IGeometry::Type::Quad);
  geomType.value("Tetrahedral", IGeometry::Type::Tetrahedral);
  geomType.value("Hexahedral", IGeometry::Type::Hexahedral);

  py::class_<IGridGeometry, IGeometry, std::shared_ptr<IGridGeometry>> iGridGeometry(mod, "IGridGeometry");
  iGridGeometry.def_property_readonly("dimensions", [](const IGridGeometry& self) { return self.getDimensions().toTuple(); });
  iGridGeometry.def_property_readonly("num_x_cells", &IGridGeometry::getNumXCells);
  iGridGeometry.def_property_readonly("num_y_cells", &IGridGeometry::getNumYCells);
  iGridGeometry.def_property_readonly("num_z_cells", &IGridGeometry::getNumZCells);

  py::class_<ImageGeom, IGridGeometry, std::shared_ptr<ImageGeom>> imageGeom(mod, "ImageGeom");
  imageGeom.def_property_readonly("spacing", [](const ImageGeom& self) { return self.getSpacing().toTuple(); });
  imageGeom.def_property_readonly("origin", [](const ImageGeom& self) { return self.getOrigin().toTuple(); });

  py::class_<RectGridGeom, IGridGeometry, std::shared_ptr<RectGridGeom>> rectGridGeom(mod, "RectGridGeom");

  py::class_<INodeGeometry0D, IGeometry, std::shared_ptr<INodeGeometry0D>> iNodeGeometry0D(mod, "INodeGeometry0D");
  py::class_<VertexGeom, INodeGeometry0D, std::shared_ptr<VertexGeom>> vertexGeom(mod, "VertexGeom");

  py::class_<INodeGeometry1D, INodeGeometry0D, std::shared_ptr<INodeGeometry1D>> iNodeGeometry1D(mod, "INodeGeometry1D");
  py::class_<EdgeGeom, INodeGeometry1D, std::shared_ptr<EdgeGeom>> edgeGeom(mod, "EdgeGeom");

  py::class_<INodeGeometry2D, INodeGeometry1D, std::shared_ptr<INodeGeometry2D>> iNodeGeometry2D(mod, "INodeGeometry2D");
  py::class_<TriangleGeom, INodeGeometry2D, std::shared_ptr<TriangleGeom>> triangleGeom(mod, "TriangleGeom");
  py::class_<QuadGeom, INodeGeometry2D, std::shared_ptr<QuadGeom>> quadGeom(mod, "QuadGeom");

  py::class_<INodeGeometry3D, INodeGeometry2D, std::shared_ptr<INodeGeometry3D>> iNodeGeometry3D(mod, "INodeGeometry3D");
  py::class_<TetrahedralGeom, INodeGeometry3D, std::shared_ptr<TetrahedralGeom>> tetrahedralGeom(mod, "TetrahedralGeom");
  py::class_<HexahedralGeom, INodeGeometry3D, std::shared_ptr<HexahedralGeom>> hexahedralGeom(mod, "HexahedralGeom");

  py::class_<DataGroup, BaseGroup, std::shared_ptr<DataGroup>> dataGroup(mod, "DataGroup");

  py::class_<AttributeMatrix, BaseGroup, std::shared_ptr<AttributeMatrix>> attributeMatrix(mod, "AttributeMatrix");

  py::class_<IArray, DataObject, std::shared_ptr<IArray>> iArray(mod, "IArray");
  iArray.def_property_readonly("tuple_shape", &IArray::getTupleShape);
  iArray.def_property_readonly("component_shape", &IArray::getComponentShape);

  py::enum_<IArray::ArrayType> iArrayArrayType(iArray, "ArrayType");
  iArrayArrayType.value("StringArray", IArray::ArrayType::StringArray);
  iArrayArrayType.value("DataArray", IArray::ArrayType::DataArray);
  iArrayArrayType.value("NeighborListArray", IArray::ArrayType::NeighborListArray);
  iArrayArrayType.value("Any", IArray::ArrayType::Any);

  py::class_<IDataArray, IArray, std::shared_ptr<IDataArray>> iDataArray(mod, "IDataArray");
  iDataArray.def_property_readonly("store", py::overload_cast<>(&IDataArray::getIDataStore));
  iDataArray.def_property_readonly("tdims", &IDataArray::getTupleShape);
  iDataArray.def_property_readonly("cdims", &IDataArray::getComponentShape);

  auto dataArrayInt8 = COMPLEX_PY_BIND_DATA_ARRAY(mod, Int8Array);
  auto dataArrayUInt8 = COMPLEX_PY_BIND_DATA_ARRAY(mod, UInt8Array);
  auto dataArrayInt16 = COMPLEX_PY_BIND_DATA_ARRAY(mod, Int16Array);
  auto dataArrayUInt16 = COMPLEX_PY_BIND_DATA_ARRAY(mod, UInt16Array);
  auto dataArrayInt32 = COMPLEX_PY_BIND_DATA_ARRAY(mod, Int32Array);
  auto dataArrayUInt32 = COMPLEX_PY_BIND_DATA_ARRAY(mod, UInt32Array);
  auto dataArrayInt64 = COMPLEX_PY_BIND_DATA_ARRAY(mod, Int64Array);
  auto dataArrayUInt64 = COMPLEX_PY_BIND_DATA_ARRAY(mod, UInt64Array);
  auto dataArrayFloat32 = COMPLEX_PY_BIND_DATA_ARRAY(mod, Float32Array);
  auto dataArrayFloat64 = COMPLEX_PY_BIND_DATA_ARRAY(mod, Float64Array);
  auto dataArrayBool = COMPLEX_PY_BIND_DATA_ARRAY(mod, BoolArray);

  rectGridGeom.def_property_readonly("x_bounds", py::overload_cast<>(&RectGridGeom::getXBoundsRef), py::return_value_policy::reference_internal);
  rectGridGeom.def_property_readonly("y_bounds", py::overload_cast<>(&RectGridGeom::getYBoundsRef), py::return_value_policy::reference_internal);
  rectGridGeom.def_property_readonly("z_bounds", py::overload_cast<>(&RectGridGeom::getZBoundsRef), py::return_value_policy::reference_internal);

  iNodeGeometry0D.def_property_readonly("vertices", py::overload_cast<>(&INodeGeometry0D::getVerticesRef), py::return_value_policy::reference_internal);
  iNodeGeometry0D.def_property_readonly("vertex_data", py::overload_cast<>(&INodeGeometry0D::getVertexAttributeMatrixRef), py::return_value_policy::reference_internal);

  iNodeGeometry1D.def_property_readonly("edges", py::overload_cast<>(&INodeGeometry1D::getEdgesRef), py::return_value_policy::reference_internal);
  iNodeGeometry1D.def_property_readonly("edge_data", py::overload_cast<>(&INodeGeometry1D::getEdgeAttributeMatrixRef), py::return_value_policy::reference_internal);

  iNodeGeometry2D.def_property_readonly("faces", py::overload_cast<>(&INodeGeometry2D::getFacesRef), py::return_value_policy::reference_internal);
  iNodeGeometry2D.def_property_readonly("face_data", py::overload_cast<>(&INodeGeometry2D::getFaceAttributeMatrixRef), py::return_value_policy::reference_internal);

  iNodeGeometry3D.def_property_readonly("polyhedra", py::overload_cast<>(&INodeGeometry3D::getPolyhedraRef), py::return_value_policy::reference_internal);
  iNodeGeometry3D.def_property_readonly("polyhedra_data", py::overload_cast<>(&INodeGeometry3D::getPolyhedraAttributeMatrixRef), py::return_value_policy::reference_internal);

  auto iDataAction = py::class_<IDataAction>(mod, "IDataAction");

  auto dataActionMode = py::enum_<IDataAction::Mode>(iDataAction, "Mode");
  dataActionMode.value("Preflight", IDataAction::Mode::Preflight);
  dataActionMode.value("Execute", IDataAction::Mode::Execute);

  iDataAction.def("apply", &IDataAction::apply);

  auto iDataCreationAction = py::class_<IDataCreationAction, IDataAction>(mod, "IDataCreationAction");

  auto iDataCreationActionArrayHandlingType = py::enum_<IDataCreationAction::ArrayHandlingType>(iDataCreationAction, "ArrayHandlingType");
  iDataCreationActionArrayHandlingType.value("Copy", IDataCreationAction::ArrayHandlingType::Copy);
  iDataCreationActionArrayHandlingType.value("Move", IDataCreationAction::ArrayHandlingType::Move);
  iDataCreationActionArrayHandlingType.value("Reference", IDataCreationAction::ArrayHandlingType::Reference);
  iDataCreationActionArrayHandlingType.value("Create", IDataCreationAction::ArrayHandlingType::Create);

  auto copyArrayInstanceAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CopyArrayInstanceAction, IDataCreationAction);
  copyArrayInstanceAction.def(py::init<const DataPath&, const DataPath&>(), "selected_data_path"_a, "created_data_path"_a);

  auto copyDataObjectAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CopyDataObjectAction, IDataCreationAction);
  copyDataObjectAction.def(py::init<const DataPath&, const DataPath&, const std::vector<DataPath>>(), "path"_a, "new_path"_a, "all_created_paths"_a);

  auto createArrayAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateArrayAction, IDataCreationAction);
  createArrayAction.def(py::init<DataType, const std::vector<usize>&, const std::vector<usize>&, const DataPath&, std::string>(), "type"_a, "t_dims"_a, "c_dims"_a, "path"_a,
                        "data_format"_a = std::string(""));

  auto createAttributeMatrixAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateAttributeMatrixAction, IDataCreationAction);
  createAttributeMatrixAction.def(py::init<const DataPath&, const AttributeMatrix::ShapeType&>(), "path"_a, "shape"_a);

  auto createDataGroupAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateDataGroupAction, IDataCreationAction);
  createDataGroupAction.def(py::init<const DataPath&>(), "path"_a);

  auto createEdgeGeometryAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateEdgeGeometryAction, IDataCreationAction);
  createEdgeGeometryAction.def(py::init<const DataPath&, size_t, size_t, const std::string&, const std::string&, const std::string&, const std::string&>(), "geometry_path"_a, "num_edges"_a,
                               "num_vertices"_a, "vertex_attribute_matrix_name"_a, "edge_attribute_matrix_name"_a, "shared_vertices_name"_a, "shared_edges_name"_a);
  createEdgeGeometryAction.def(py::init<const DataPath&, const DataPath&, const DataPath&, const std::string&, const std::string&, const IDataCreationAction::ArrayHandlingType&>(), "geometry_path"_a,
                               "input_vertices_array_path"_a, "input_edges_array_path"_a, "vertex_attribute_matrix_name"_a, "edge_attribute_matrix_name"_a, "array_type"_a);

  auto createTriangleGeometryAction = COMPLEX_PY_BIND_CREATE_GEOMETRY_2D_ACTION(mod, CreateTriangleGeometryAction);
  auto createQuadGeometryAction = COMPLEX_PY_BIND_CREATE_GEOMETRY_2D_ACTION(mod, CreateQuadGeometryAction);

  auto createTetrahedralGeometryAction = COMPLEX_PY_BIND_CREATE_GEOMETRY_3D_ACTION(mod, CreateTetrahedralGeometryAction);
  auto createHexahedralGeometryAction = COMPLEX_PY_BIND_CREATE_GEOMETRY_3D_ACTION(mod, CreateHexahedralGeometryAction);

  auto createImageGeometryAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateImageGeometryAction, IDataCreationAction);
  createImageGeometryAction.def(
      py::init<const DataPath&, const CreateImageGeometryAction::DimensionType&, const CreateImageGeometryAction::OriginType&, const CreateImageGeometryAction::SpacingType&, const std::string&>(),
      "path"_a, "dims"_a, "origin"_a, "spacing"_a, "cell_attribute_matrix_name"_a);

  auto createNeighborListAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateNeighborListAction, IDataCreationAction);
  createNeighborListAction.def(py::init<DataType, usize, const DataPath&>(), "type"_a, "tuple_count"_a, "path"_a);

  auto createRectGridGeometryAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateRectGridGeometryAction, IDataCreationAction);
  createRectGridGeometryAction.def(py::init<const DataPath&, usize, usize, usize, const std::string&, const std::string&, const std::string&, const std::string&>(), "path"_a, "x_bounds_dim"_a,
                                   "y_bounds_dim"_a, "z_bounds_dim"_a, "cell_attribute_matrix_name"_a, "x_bounds_name"_a, "y_bounds_name"_a, "z_bounds_name"_a);
  createRectGridGeometryAction.def(py::init<const DataPath&, const DataPath&, const DataPath&, const DataPath&, const std::string&, const IDataCreationAction::ArrayHandlingType&>(), "path"_a,
                                   "input_x_bounds_path"_a, "input_y_bounds_path"_a, "input_z_bounds_path"_a, "cell_attribute_matrix_name"_a, "array_type"_a);

  auto createStringArrayAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateStringArrayAction, IDataCreationAction);
  createStringArrayAction.def(py::init<const std::vector<usize>&, const DataPath&, const std::string&>(), "t_dims"_a, "path"_a, "initialize_value"_a = std::string(""));

  auto createVertexGeometryAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, CreateVertexGeometryAction, IDataCreationAction);
  createVertexGeometryAction.def(py::init<const DataPath&, IGeometry::MeshIndexType, const std::string&, const std::string&>(), "geometry_path"_a, "num_vertices"_a, "vertex_attribute_matrix_name"_a,
                                 "shared_vertex_list_name"_a);
  createVertexGeometryAction.def(py::init<const DataPath&, const DataPath&, const std::string&, const IDataCreationAction::ArrayHandlingType&>(), "geometry_path"_a, "input_vertices_array_path"_a,
                                 "vertex_attribute_matrix_name"_a, "array_type"_a);

  auto deleteDataAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, DeleteDataAction, IDataAction);

  auto deleteDataActionType = py::enum_<DeleteDataAction::DeleteType>(deleteDataAction, "DeleteType");
  deleteDataActionType.value("JustObject", DeleteDataAction::DeleteType::JustObject);

  deleteDataAction.def(py::init<const DataPath&, DeleteDataAction::DeleteType>(), "path"_a, "type"_a);

  auto importH5ObjectPathsAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, ImportH5ObjectPathsAction, IDataCreationAction);
  importH5ObjectPathsAction.def(py::init<const std::filesystem::path&, const ImportH5ObjectPathsAction::PathsType&>(), "import_file"_a, "paths"_a);

  auto moveDataAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, MoveDataAction, IDataAction);
  moveDataAction.def(py::init<const DataPath&, const DataPath&>(), "path"_a, "new_parent_path"_a);

  auto renameDataAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, RenameDataAction, IDataAction);
  renameDataAction.def(py::init<const DataPath&, const std::string&>(), "path"_a, "new_name"_a);

  auto updateImageGeomAction = COMPLEX_PY_BIND_CLASS_VARIADIC(mod, UpdateImageGeomAction, IDataAction);
  updateImageGeomAction.def(py::init<const std::optional<FloatVec3>&, const std::optional<FloatVec3>&, const DataPath&>(), "origin"_a, "spacing"_a, "path"_a);

  auto outputActions = py::class_<OutputActions>(mod, "OutputActions");
  outputActions.def(py::init<>());
  outputActions.def("append_action", [](OutputActions& self, const IDataAction& action) { self.appendAction(action.clone()); });
  outputActions.def("append_deferred_action", [](OutputActions& self, const IDataAction& action) { self.appendDeferredAction(action.clone()); });

  auto arrayCreationParameter = COMPLEX_PY_BIND_PARAMETER(mod, ArrayCreationParameter);
  auto arraySelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, ArraySelectionParameter);
  auto arrayThresholdsParameter = COMPLEX_PY_BIND_PARAMETER(mod, ArrayThresholdsParameter);
  auto attributeMatrixSelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, AttributeMatrixSelectionParameter);
  auto boolParameter = COMPLEX_PY_BIND_PARAMETER(mod, BoolParameter);
  auto calculatorParameter = COMPLEX_PY_BIND_PARAMETER(mod, CalculatorParameter);
  auto choicesParameter = COMPLEX_PY_BIND_PARAMETER(mod, ChoicesParameter);
  auto dataGroupCreationParameter = COMPLEX_PY_BIND_PARAMETER(mod, DataGroupCreationParameter);
  auto dataGroupSelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, DataGroupSelectionParameter);
  auto dataObjectNameParameter = COMPLEX_PY_BIND_PARAMETER(mod, DataObjectNameParameter);
  auto dataPathSelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, DataPathSelectionParameter);
  auto dataStoreFormatParameter = COMPLEX_PY_BIND_PARAMETER(mod, DataStoreFormatParameter);
  auto dataTypeParameter = COMPLEX_PY_BIND_PARAMETER(mod, DataTypeParameter);
  auto dream3dImportParameter = COMPLEX_PY_BIND_PARAMETER(mod, Dream3dImportParameter);
  auto dynamicTableParameter = COMPLEX_PY_BIND_PARAMETER(mod, DynamicTableParameter);
  auto ensembleInfoParameter = COMPLEX_PY_BIND_PARAMETER(mod, EnsembleInfoParameter);
  auto fileSystemPathParameter = COMPLEX_PY_BIND_PARAMETER(mod, FileSystemPathParameter);
  auto generateColorTableParameter = COMPLEX_PY_BIND_PARAMETER(mod, GenerateColorTableParameter);
  auto generatedFileListParameter = COMPLEX_PY_BIND_PARAMETER(mod, GeneratedFileListParameter);
  auto geometrySelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, GeometrySelectionParameter);
  auto importCSVDataParameter = COMPLEX_PY_BIND_PARAMETER(mod, ImportCSVDataParameter);
  auto importHDF5DatasetParameter = COMPLEX_PY_BIND_PARAMETER(mod, ImportHDF5DatasetParameter);
  auto multiArraySelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, MultiArraySelectionParameter);
  auto multiPathSelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, MultiPathSelectionParameter);
  auto neighborListSelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, NeighborListSelectionParameter);
  auto int8Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, Int8Parameter);
  auto uInt8Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, UInt8Parameter);
  auto int16Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, Int16Parameter);
  auto uInt16Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, UInt16Parameter);
  auto int32Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, Int32Parameter);
  auto uInt32Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, UInt32Parameter);
  auto int64Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, Int64Parameter);
  auto uInt64Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, UInt64Parameter);
  auto float32Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, Float32Parameter);
  auto float64Parameter = COMPLEX_PY_BIND_NUMBER_PARAMETER(mod, Float64Parameter);
  auto numericTypeParameter = COMPLEX_PY_BIND_PARAMETER(mod, NumericTypeParameter);
  auto stringParameter = COMPLEX_PY_BIND_PARAMETER(mod, StringParameter);
  auto vectorInt8Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorInt8Parameter);
  auto vectorUInt8Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorUInt8Parameter);
  auto vectorInt16Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorInt16Parameter);
  auto vectorUInt16Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorUInt16Parameter);
  auto vectorInt32Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorInt32Parameter);
  auto vectorUInt32Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorUInt32Parameter);
  auto vectorInt64Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorInt64Parameter);
  auto vectorUInt64Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorUInt64Parameter);
  auto vectorFloat32Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorFloat32Parameter);
  auto vectorFloat64Parameter = COMPLEX_PY_BIND_VECTOR_PARAMETER(mod, VectorFloat64Parameter);

  py::class_<ImportHDF5DatasetParameter::DatasetImportInfo> datasetImportInfo(importHDF5DatasetParameter, "DatasetImportInfo");
  datasetImportInfo.def(py::init<>());
  datasetImportInfo.def_readwrite("dataset_path", &ImportHDF5DatasetParameter::DatasetImportInfo::dataSetPath);
  datasetImportInfo.def_readwrite("component_dims", &ImportHDF5DatasetParameter::DatasetImportInfo::componentDimensions);
  datasetImportInfo.def_readwrite("tuple_dims", &ImportHDF5DatasetParameter::DatasetImportInfo::tupleDimensions);

  py::class_<ImportHDF5DatasetParameter::ValueType> importHdf5DatasetInfo(importHDF5DatasetParameter, "ValueType");
  importHdf5DatasetInfo.def(py::init<>());
  importHdf5DatasetInfo.def_readwrite("parent", &ImportHDF5DatasetParameter::ValueType::parent);
  importHdf5DatasetInfo.def_readwrite("input_file", &ImportHDF5DatasetParameter::ValueType::inputFile);
  importHdf5DatasetInfo.def_readwrite("datasets", &ImportHDF5DatasetParameter::ValueType::datasets);
  importHdf5DatasetInfo.def("__repr__", [](const ImportHDF5DatasetParameter::ValueType& self) { return "ImportHDF5DatasetParameter.ValueType()"; });

  py::enum_<GeneratedFileListParameter::Ordering> generatedFileListOrdering(generatedFileListParameter, "Ordering");
  generatedFileListOrdering.value("LowToHigh", GeneratedFileListParameter::Ordering::LowToHigh);
  generatedFileListOrdering.value("HighToLow", GeneratedFileListParameter::Ordering::HighToLow);

  py::class_<GeneratedFileListParameter::ValueType> generatedFileList(generatedFileListParameter, "ValueType");
  generatedFileList.def(py::init<>());
  generatedFileList.def_readwrite("start_index", &GeneratedFileListParameter::ValueType::startIndex);
  generatedFileList.def_readwrite("end_index", &GeneratedFileListParameter::ValueType::endIndex);
  generatedFileList.def_readwrite("increment_index", &GeneratedFileListParameter::ValueType::incrementIndex);
  generatedFileList.def_readwrite("padding_digits", &GeneratedFileListParameter::ValueType::paddingDigits);
  generatedFileList.def_readwrite("ordering", &GeneratedFileListParameter::ValueType::ordering);
  generatedFileList.def_readwrite("input_path", &GeneratedFileListParameter::ValueType::inputPath);
  generatedFileList.def_readwrite("file_prefix", &GeneratedFileListParameter::ValueType::filePrefix);
  generatedFileList.def_readwrite("file_suffix", &GeneratedFileListParameter::ValueType::fileSuffix);
  generatedFileList.def_readwrite("file_extension", &GeneratedFileListParameter::ValueType::fileExtension);
  generatedFileList.def("generate", &GeneratedFileListParameter::ValueType::generate);
  generatedFileList.def("generate_and_validate", &GeneratedFileListParameter::ValueType::generateAndValidate);

  py::class_<Dream3dImportParameter::ImportData> dream3dImportData(dream3dImportParameter, "ImportData");
  dream3dImportData.def(py::init<>());
  dream3dImportData.def_readwrite("file_path", &Dream3dImportParameter::ImportData::FilePath);
  dream3dImportData.def_readwrite("data_paths", &Dream3dImportParameter::ImportData::DataPaths);
  dream3dImportData.def("__repr__", [](const Dream3dImportParameter::ImportData& self) { return "Dream3dImportParameter.ImportData()"; });

  auto arraySelectionParamterDataLocation = py::enum_<ArraySelectionParameter::DataLocation>(arraySelectionParameter, "DataLocation");
  arraySelectionParamterDataLocation.value("Any", ArraySelectionParameter::DataLocation::Any);
  arraySelectionParamterDataLocation.value("InMemory", ArraySelectionParameter::DataLocation::InMemory);
  arraySelectionParamterDataLocation.value("OutOfCore", ArraySelectionParameter::DataLocation::OutOfCore);

  auto dynamicTableInfo = py::class_<DynamicTableInfo>(mod, "DynamicTableInfo");

  auto dynamicTableVectorInfo = py::class_<DynamicTableInfo::VectorInfo>(dynamicTableInfo, "VectorInfo");

  auto dynamicTableStaticVectorInfo = py::class_<DynamicTableInfo::StaticVectorInfo>(dynamicTableInfo, "StaticVectorInfo");
  dynamicTableStaticVectorInfo.def(py::init<usize>(), "size"_a);
  dynamicTableStaticVectorInfo.def(py::init<DynamicTableInfo::HeadersListType>(), "headers"_a);

  auto dynamicTableDynamicVectorInfo = py::class_<DynamicTableInfo::DynamicVectorInfo>(dynamicTableInfo, "DynamicVectorInfo");
  dynamicTableDynamicVectorInfo.def(py::init<usize, usize, std::string>(), "min_size"_a, "default_size"_a, "header_template"_a);
  dynamicTableDynamicVectorInfo.def(py::init<usize, std::string>(), "min_size"_a, "header_template"_a);

  dynamicTableVectorInfo.def(py::init<DynamicTableInfo::StaticVectorInfo>(), "vector_info"_a);
  dynamicTableVectorInfo.def(py::init<DynamicTableInfo::DynamicVectorInfo>(), "vector_info"_a);

  py::implicitly_convertible<DynamicTableInfo::StaticVectorInfo, DynamicTableInfo::VectorInfo>();
  py::implicitly_convertible<DynamicTableInfo::DynamicVectorInfo, DynamicTableInfo::VectorInfo>();

  dynamicTableInfo.def(py::init<>());
  dynamicTableInfo.def(py::init<DynamicTableInfo::VectorInfo, DynamicTableInfo::VectorInfo>(), "rows_info"_a, "cols_info"_a);
  dynamicTableInfo.def("set_rows_info", &DynamicTableInfo::setRowsInfo, "info"_a);
  dynamicTableInfo.def("set_cols_info", &DynamicTableInfo::setColsInfo, "info"_a);

  auto fileSystemPathParameterPathType = py::enum_<FileSystemPathParameter::PathType>(fileSystemPathParameter, "PathType");
  fileSystemPathParameterPathType.value("InputFile", FileSystemPathParameter::PathType::InputFile);
  fileSystemPathParameterPathType.value("InputDir", FileSystemPathParameter::PathType::InputDir);
  fileSystemPathParameterPathType.value("OutputFile", FileSystemPathParameter::PathType::OutputFile);
  fileSystemPathParameterPathType.value("OutputDir", FileSystemPathParameter::PathType::OutputDir);

  auto calculatorParameterAngleUnits = py::enum_<CalculatorParameter::AngleUnits>(calculatorParameter, "AngleUnits");
  calculatorParameterAngleUnits.value("Radians", CalculatorParameter::AngleUnits::Radians);
  calculatorParameterAngleUnits.value("Degrees", CalculatorParameter::AngleUnits::Degrees);

  auto calculatorParameterValueType = py::class_<CalculatorParameter::ValueType>(calculatorParameter, "ValueType");
  calculatorParameterValueType.def(py::init<DataPath, std::string, CalculatorParameter::AngleUnits>());
  calculatorParameterValueType.def_readwrite("selected_group", &CalculatorParameter::ValueType::m_SelectedGroup);
  calculatorParameterValueType.def_readwrite("equation", &CalculatorParameter::ValueType::m_Equation);
  calculatorParameterValueType.def_readwrite("units", &CalculatorParameter::ValueType::m_Units);

  BindParameterConstructor(arrayCreationParameter);

  arraySelectionParameter.def(py::init<const std::string&, const std::string&, const std::string&, const ArraySelectionParameter::ValueType&, const ArraySelectionParameter::AllowedTypes&,
                                       ArraySelectionParameter::AllowedComponentShapes, ArraySelectionParameter::DataLocation>(),
                              "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "allowed_types"_a, "required_comps"_a = ArraySelectionParameter::AllowedComponentShapes{},
                              "location"_a = ArraySelectionParameter::DataLocation::Any);

  arrayThresholdsParameter.def(py::init<const std::string&, const std::string&, const std::string&, const ArrayThresholdsParameter::ValueType&, ArrayThresholdsParameter::AllowedComponentShapes>(),
                               "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "required_comps"_a = ArrayThresholdsParameter::AllowedComponentShapes{});

  BindParameterConstructor(attributeMatrixSelectionParameter);

  BindParameterConstructor(boolParameter);

  BindParameterConstructor(calculatorParameter);

  choicesParameter.def(py::init<const std::string&, const std::string&, const std::string&, ChoicesParameter::ValueType, const ChoicesParameter::Choices&>());

  BindParameterConstructor(dataGroupCreationParameter);

  dataGroupSelectionParameter.def(
      py::init<const std::string&, const std::string&, const std::string&, const DataGroupSelectionParameter::ValueType&, const DataGroupSelectionParameter::AllowedTypes&>(), "name"_a, "human_name"_a,
      "help_text"_a, "default_value"_a, "allowed_types"_a);

  BindParameterConstructor(dataObjectNameParameter);

  BindParameterConstructor(dataPathSelectionParameter);

  BindParameterConstructor(dataStoreFormatParameter);

  BindParameterConstructor(dataTypeParameter);

  BindParameterConstructor(dream3dImportParameter);

  dynamicTableParameter.def(py::init<const std::string&, const std::string&, const std::string&, const DynamicTableParameter::ValueType&, const DynamicTableInfo&>(), "name"_a, "human_name"_a,
                            "help_text"_a, "default_value"_a, "table_info"_a);
  dynamicTableParameter.def(py::init<const std::string&, const std::string&, const std::string&, const DynamicTableInfo&>(), "name"_a, "human_name"_a, "help_text"_a, "table_info"_a);

  BindParameterConstructor(ensembleInfoParameter);

  fileSystemPathParameter.def(py::init<const std::string&, const std::string&, const std::string&, const FileSystemPathParameter::ValueType&, const FileSystemPathParameter::ExtensionsType&,
                                       FileSystemPathParameter::PathType, bool>(),
                              "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "extensions_type"_a, "path_type"_a, "accept_all_extensions"_a = false);

  BindParameterConstructor(generateColorTableParameter);

  BindParameterConstructor(generatedFileListParameter);

  geometrySelectionParameter.def(py::init<const std::string&, const std::string&, const std::string&, const GeometrySelectionParameter::ValueType&, const GeometrySelectionParameter::AllowedTypes&>(),
                                 "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "allowed_types"_a);

  BindParameterConstructor(importCSVDataParameter);

  BindParameterConstructor(importHDF5DatasetParameter);

  multiArraySelectionParameter.def(
      py::init<const std::string&, const std::string&, const std::string&, const MultiArraySelectionParameter::ValueType&, const MultiArraySelectionParameter::AllowedTypes&,
               const MultiArraySelectionParameter::AllowedDataTypes&, MultiArraySelectionParameter::AllowedComponentShapes>(),
      "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "allowed_types"_a, "allowed_data_types"_a, "required_comps"_a = MultiArraySelectionParameter::AllowedComponentShapes{});

  BindParameterConstructor(multiPathSelectionParameter);

  neighborListSelectionParameter.def(
      py::init<const std::string&, const std::string&, const std::string&, const NeighborListSelectionParameter::ValueType&, const NeighborListSelectionParameter::AllowedTypes&>(), "name"_a,
      "human_name"_a, "help_text"_a, "default_value"_a, "allowed_types"_a);

  BindParameterConstructor(numericTypeParameter);

  BindParameterConstructor(stringParameter);

  py::class_<IFilter /*, PyFilter*/> filter(mod, "IFilter");

  py::class_<IFilter::Message> filterMessage(filter, "Message");

  py::enum_<IFilter::Message::Type> filterMessageType(filterMessage, "Type");
  filterMessageType.value("Info", IFilter::Message::Type::Info);
  filterMessageType.value("Debug", IFilter::Message::Type::Debug);
  filterMessageType.value("Progress", IFilter::Message::Type::Progress);
  filterMessageType.value("Warning", IFilter::Message::Type::Warning);
  filterMessageType.value("Error", IFilter::Message::Type::Error);

  filterMessage.def(py::init<>());
  filterMessage.def(py::init<IFilter::Message::Type, const std::string&>());
  filterMessage.def_readwrite("type", &IFilter::Message::type);
  filterMessage.def_readwrite("message", &IFilter::Message::message);

  py::class_<IFilter::MessageHandler> messageHandler(filter, "MessageHandler");
  messageHandler.def(py::init<>());
  messageHandler.def_readwrite("callback", &IFilter::MessageHandler::m_Callback);
  messageHandler.def("__call__", [](const IFilter::MessageHandler& self, const IFilter::Message& message) { self(message); });

  py::class_<IFilter::PreflightValue> preflightValue(filter, "PreflightValue");
  preflightValue.def(py::init<>());
  preflightValue.def_readwrite("name", &IFilter::PreflightValue::name);
  preflightValue.def_readwrite("value", &IFilter::PreflightValue::value);

  py::class_<IFilter::PreflightResult> preflightResult(filter, "PreflightResult");
  preflightResult.def(py::init<>());
  preflightResult.def(py::init<>([](std::optional<OutputActions> outputActions, std::optional<std::vector<Error>> errors, std::optional<std::vector<Warning>> warnings,
                                    std::optional<std::vector<IFilter::PreflightValue>> preflightValues) {
                        IFilter::PreflightResult result{};
                        if(outputActions.has_value() && errors.has_value())
                        {
                          throw std::invalid_argument("IFilter.PreflightResult cannot be constructed with both output_actions and errors set");
                        }
                        if(outputActions.has_value())
                        {
                          result.outputActions = {std::move(*outputActions)};
                        }
                        if(errors.has_value())
                        {
                          result.outputActions = {nonstd::make_unexpected(std::move(*errors))};
                        }
                        if(warnings.has_value())
                        {
                          result.outputActions.warnings() = std::move(*warnings);
                        }
                        if(preflightValues.has_value())
                        {
                          result.outputValues = std::move(*preflightValues);
                        }
                        return result;
                      }),
                      "output_actions"_a = py::none(), "errors"_a = py::none(), "warnings"_a = py::none(), "preflight_values"_a = py::none());
  preflightResult.def("set_errors", [](IFilter::PreflightResult& self, std::vector<Error> errors) { self.outputActions.m_Expected = nonstd::make_unexpected(std::move(errors)); });
  // preflightResult.def_readwrite("output_actions", &IFilter::PreflightResult::outputActions);
  preflightResult.def("get_result", [](IFilter::PreflightResult& self) {
    if(self.outputActions.valid())
    {
      return std::vector<Error>{};
    }
    return self.outputActions.errors();
  });
  preflightResult.def_readwrite("output_values", &IFilter::PreflightResult::outputValues);

  py::class_<IFilter::ExecuteResult> executeResult(filter, "ExecuteResult");
  executeResult.def(py::init<>());
  executeResult.def_property_readonly("errors", [](const IFilter::ExecuteResult& self) { return self.result.errors(); });
  executeResult.def_property_readonly("warnings", [](const IFilter::ExecuteResult& self) { return self.result.warnings(); });
  executeResult.def("__bool__", [](const IFilter::ExecuteResult& self) { return self.result.valid(); });
  executeResult.def("__repr__", [](const IFilter::ExecuteResult& self) {
    return fmt::format("<complex.IFilter.ExecuteResult(errors=[{}], warnings=[{}])>", fmt::join(self.result.valid() ? ErrorCollection{} : self.result.errors(), ", "),
                       fmt::join(self.result.warnings(), ", "));
  });

  filter.def("name", &IFilter::name);
  filter.def("uuid", &IFilter::uuid);
  filter.def("human_name", &IFilter::humanName);
  filter.def("preflight2", [internals](const IFilter& self, DataStructure& dataStructure, const py::kwargs& kwargs) {
    Arguments convertedArgs = ConvertDictToArgs(*internals, self.parameters(), kwargs);
    IFilter::PreflightResult result = self.preflight(dataStructure, convertedArgs, CreatePyMessageHandler());
    return result;
  });
  filter.def(
      "execute2",
      [internals](const IFilter& self, DataStructure& dataStructure, const py::kwargs& kwargs) {
        Arguments convertedArgs = ConvertDictToArgs(*internals, self.parameters(), kwargs);
        IFilter::ExecuteResult result = self.execute(dataStructure, convertedArgs, nullptr, CreatePyMessageHandler());
        return result;
      },
      "data_structure"_a, "Executes the filter");

  py::class_<Pipeline, AbstractPipelineNode, std::shared_ptr<Pipeline>> pipeline(mod, "Pipeline");
  pipeline.def(py::init<const std::string&>(), "name"_a = std::string("Untitled Pipeline"));
  pipeline.def_static(
      "from_file",
      [](const std::filesystem::path& path) {
        Result<Pipeline> pipelineResult = Pipeline::FromFile(path);
        if(pipelineResult.invalid())
        {
          throw std::runtime_error(fmt::format("Errors={}", pipelineResult.errors()));
        }
        return pipelineResult.value();
      },
      "path"_a);
  pipeline.def("execute", [](Pipeline& self, DataStructure& dataStructure) { return self.execute(dataStructure, false); });
  pipeline.def(
      "__getitem__", [](Pipeline& self, Pipeline::index_type index) { return self.at(index); }, py::return_value_policy::reference_internal);
  pipeline.def("__len__", &Pipeline::size);
  pipeline.def(
      "__iter__", [](Pipeline& self) { return py::make_iterator(self.begin(), self.end()); }, py::keep_alive<0, 1>());

  pipelineFilter.def("get_args", [internals](PipelineFilter& self) { return ConvertArgsToDict(*internals, self.getFilter()->parameters(), self.getArguments()); });
  pipelineFilter.def(
      "get_filter", [](PipelineFilter& self) { return self.getFilter(); }, py::return_value_policy::reference_internal);

  py::class_<PyFilter, IFilter> pyFilter(mod, "PyFilter");
  pyFilter.def(py::init<>([](py::object object) { return std::make_unique<PyFilter>(std::move(object)); }));

  // Parameter value types conversions must be registered after the value types are bound
  // but before the filters are bound so that the filters signatures are properly generated.

  internals->addConversion<ArrayCreationParameter>();
  internals->addConversion<ArraySelectionParameter>();
  internals->addConversion<ArrayThresholdsParameter>();
  internals->addConversion<AttributeMatrixSelectionParameter>();
  internals->addConversion<BoolParameter>();
  internals->addConversion<CalculatorParameter>();
  internals->addConversion<ChoicesParameter>();
  internals->addConversion<DataGroupCreationParameter>();
  internals->addConversion<DataGroupSelectionParameter>();
  internals->addConversion<DataObjectNameParameter>();
  internals->addConversion<DataPathSelectionParameter>();
  internals->addConversion<DataStoreFormatParameter>();
  internals->addConversion<DataTypeParameter>();
  internals->addConversion<Dream3dImportParameter>();
  internals->addConversion<DynamicTableParameter>();
  internals->addConversion<EnsembleInfoParameter>();
  internals->addConversion<FileSystemPathParameter>();
  internals->addConversion<GenerateColorTableParameter>();
  internals->addConversion<GeneratedFileListParameter>();
  internals->addConversion<GeometrySelectionParameter>();
  internals->addConversion<ImportCSVDataParameter>();
  internals->addConversion<ImportHDF5DatasetParameter>();
  internals->addConversion<MultiArraySelectionParameter>();
  internals->addConversion<MultiPathSelectionParameter>();
  internals->addConversion<NeighborListSelectionParameter>();
  internals->addConversion<Int8Parameter>();
  internals->addConversion<UInt8Parameter>();
  internals->addConversion<Int16Parameter>();
  internals->addConversion<UInt16Parameter>();
  internals->addConversion<Int32Parameter>();
  internals->addConversion<UInt32Parameter>();
  internals->addConversion<Int64Parameter>();
  internals->addConversion<UInt64Parameter>();
  internals->addConversion<Float32Parameter>();
  internals->addConversion<Float64Parameter>();
  internals->addConversion<NumericTypeParameter>();
  internals->addConversion<StringParameter>();
  internals->addConversion<VectorInt8Parameter>();
  internals->addConversion<VectorUInt8Parameter>();
  internals->addConversion<VectorInt16Parameter>();
  internals->addConversion<VectorUInt16Parameter>();
  internals->addConversion<VectorInt32Parameter>();
  internals->addConversion<VectorUInt32Parameter>();
  internals->addConversion<VectorInt64Parameter>();
  internals->addConversion<VectorUInt64Parameter>();
  internals->addConversion<VectorFloat32Parameter>();
  internals->addConversion<VectorFloat64Parameter>();

  auto abaqusHexahedronWriterFilter = BindFilter<AbaqusHexahedronWriterFilter>(mod, *internals);
  auto addBadDataFilter = BindFilter<AddBadDataFilter>(mod, *internals);
  auto alignGeometries = BindFilter<AlignGeometries>(mod, *internals);
  auto alignSectionsFeatureCentroidFilter = BindFilter<AlignSectionsFeatureCentroidFilter>(mod, *internals);
  auto alignSectionsListFilter = BindFilter<AlignSectionsListFilter>(mod, *internals);
  auto appendImageGeometryZSliceFilter = BindFilter<AppendImageGeometryZSliceFilter>(mod, *internals);
  auto applyTransformationToGeometryFilter = BindFilter<ApplyTransformationToGeometryFilter>(mod, *internals);
  auto approximatePointCloudHull = BindFilter<ApproximatePointCloudHull>(mod, *internals);
  auto arrayCalculatorFilter = BindFilter<ArrayCalculatorFilter>(mod, *internals);
  auto avizoRectilinearCoordinateWriterFilter = BindFilter<AvizoRectilinearCoordinateWriterFilter>(mod, *internals);
  auto avizoUniformCoordinateWriterFilter = BindFilter<AvizoUniformCoordinateWriterFilter>(mod, *internals);
  auto calculateArrayHistogramFilter = BindFilter<CalculateArrayHistogramFilter>(mod, *internals);
  auto calculateFeatureSizesFilter = BindFilter<CalculateFeatureSizesFilter>(mod, *internals);
  auto calculateTriangleAreasFilter = BindFilter<CalculateTriangleAreasFilter>(mod, *internals);
  auto changeAngleRepresentation = BindFilter<ChangeAngleRepresentation>(mod, *internals);
  auto combineAttributeArraysFilter = BindFilter<CombineAttributeArraysFilter>(mod, *internals);
  auto combineStlFilesFilter = BindFilter<CombineStlFilesFilter>(mod, *internals);
  auto computeFeatureRectFilter = BindFilter<ComputeFeatureRectFilter>(mod, *internals);
  auto computeMomentInvariants2DFilter = BindFilter<ComputeMomentInvariants2DFilter>(mod, *internals);
  auto conditionalSetValue = BindFilter<ConditionalSetValue>(mod, *internals);
  auto convertColorToGrayScaleFilter = BindFilter<ConvertColorToGrayScaleFilter>(mod, *internals);
  auto convertDataFilter = BindFilter<ConvertDataFilter>(mod, *internals);
  auto copyDataObjectFilter = BindFilter<CopyDataObjectFilter>(mod, *internals);
  auto copyFeatureArrayToElementArray = BindFilter<CopyFeatureArrayToElementArray>(mod, *internals);
  auto createAttributeMatrixFilter = BindFilter<CreateAttributeMatrixFilter>(mod, *internals);
  auto createDataArray = BindFilter<CreateDataArray>(mod, *internals);
  auto createDataGroup = BindFilter<CreateDataGroup>(mod, *internals);
  auto createFeatureArrayFromElementArray = BindFilter<CreateFeatureArrayFromElementArray>(mod, *internals);
  auto createGeometryFilter = BindFilter<CreateGeometryFilter>(mod, *internals);
  auto createImageGeometry = BindFilter<CreateImageGeometry>(mod, *internals);
  auto cropImageGeometry = BindFilter<CropImageGeometry>(mod, *internals);
  auto cropVertexGeometry = BindFilter<CropVertexGeometry>(mod, *internals);
  auto deleteData = BindFilter<DeleteData>(mod, *internals);
  auto erodeDilateBadDataFilter = BindFilter<ErodeDilateBadDataFilter>(mod, *internals);
  auto erodeDilateCoordinationNumberFilter = BindFilter<ErodeDilateCoordinationNumberFilter>(mod, *internals);
  auto erodeDilateMaskFilter = BindFilter<ErodeDilateMaskFilter>(mod, *internals);
  auto executeProcessFilter = BindFilter<ExecuteProcessFilter>(mod, *internals);
  auto exportDREAM3DFilter = BindFilter<ExportDREAM3DFilter>(mod, *internals);
  auto extractComponentAsArrayFilter = BindFilter<ExtractComponentAsArrayFilter>(mod, *internals);
  auto extractInternalSurfacesFromTriangleGeometry = BindFilter<ExtractInternalSurfacesFromTriangleGeometry>(mod, *internals);
  auto extractVertexGeometryFilter = BindFilter<ExtractVertexGeometryFilter>(mod, *internals);
  auto featureDataCSVWriterFilter = BindFilter<FeatureDataCSVWriterFilter>(mod, *internals);
  auto fillBadDataFilter = BindFilter<FillBadDataFilter>(mod, *internals);
  auto findArrayStatisticsFilter = BindFilter<FindArrayStatisticsFilter>(mod, *internals);
  auto findBiasedFeaturesFilter = BindFilter<FindBiasedFeaturesFilter>(mod, *internals);
  auto findBoundaryCellsFilter = BindFilter<FindBoundaryCellsFilter>(mod, *internals);
  auto findBoundaryElementFractionsFilter = BindFilter<FindBoundaryElementFractionsFilter>(mod, *internals);
  auto findDifferencesMap = BindFilter<FindDifferencesMap>(mod, *internals);
  auto findEuclideanDistMapFilter = BindFilter<FindEuclideanDistMapFilter>(mod, *internals);
  auto findFeatureCentroidsFilter = BindFilter<FindFeatureCentroidsFilter>(mod, *internals);
  auto findFeatureClusteringFilter = BindFilter<FindFeatureClusteringFilter>(mod, *internals);
  auto findFeaturePhasesBinaryFilter = BindFilter<FindFeaturePhasesBinaryFilter>(mod, *internals);
  auto findFeaturePhasesFilter = BindFilter<FindFeaturePhasesFilter>(mod, *internals);
  auto findLargestCrossSectionsFilter = BindFilter<FindLargestCrossSectionsFilter>(mod, *internals);
  auto findNeighborhoodsFilter = BindFilter<FindNeighborhoodsFilter>(mod, *internals);
  auto findNeighborListStatistics = BindFilter<FindNeighborListStatistics>(mod, *internals);
  auto findNeighbors = BindFilter<FindNeighbors>(mod, *internals);
  auto findNumFeaturesFilter = BindFilter<FindNumFeaturesFilter>(mod, *internals);
  auto findSurfaceAreaToVolumeFilter = BindFilter<FindSurfaceAreaToVolumeFilter>(mod, *internals);
  auto findSurfaceFeatures = BindFilter<FindSurfaceFeatures>(mod, *internals);
  auto findTriangleGeomCentroidsFilter = BindFilter<FindTriangleGeomCentroidsFilter>(mod, *internals);
  auto findTriangleGeomSizesFilter = BindFilter<FindTriangleGeomSizesFilter>(mod, *internals);
  auto findVertexToTriangleDistancesFilter = BindFilter<FindVertexToTriangleDistancesFilter>(mod, *internals);
  auto findVolFractionsFilter = BindFilter<FindVolFractionsFilter>(mod, *internals);
  auto generateColorTableFilter = BindFilter<GenerateColorTableFilter>(mod, *internals);
  auto identifySample = BindFilter<IdentifySample>(mod, *internals);
  auto imageContouringFilter = BindFilter<ImageContouringFilter>(mod, *internals);
  auto importBinaryCTNorthstarFilter = BindFilter<ImportBinaryCTNorthstarFilter>(mod, *internals);
  auto importCSVDataFilter = BindFilter<ImportCSVDataFilter>(mod, *internals);
  auto importDREAM3DFilter = BindFilter<ImportDREAM3DFilter>(mod, *internals);
  auto importHDF5Dataset = BindFilter<ImportHDF5Dataset>(mod, *internals);
  auto importTextFilter = BindFilter<ImportTextFilter>(mod, *internals);
  auto importVolumeGraphicsFileFilter = BindFilter<ImportVolumeGraphicsFileFilter>(mod, *internals);
  auto initializeData = BindFilter<InitializeData>(mod, *internals);
  auto interpolatePointCloudToRegularGridFilter = BindFilter<InterpolatePointCloudToRegularGridFilter>(mod, *internals);
  auto iterativeClosestPointFilter = BindFilter<IterativeClosestPointFilter>(mod, *internals);
  auto laplacianSmoothingFilter = BindFilter<LaplacianSmoothingFilter>(mod, *internals);
  auto mapPointCloudToRegularGridFilter = BindFilter<MapPointCloudToRegularGridFilter>(mod, *internals);
  auto minNeighbors = BindFilter<MinNeighbors>(mod, *internals);
  auto moveData = BindFilter<MoveData>(mod, *internals);
  auto multiThresholdObjects = BindFilter<MultiThresholdObjects>(mod, *internals);
  auto nearestPointFuseRegularGridsFilter = BindFilter<NearestPointFuseRegularGridsFilter>(mod, *internals);
  auto partitionGeometryFilter = BindFilter<PartitionGeometryFilter>(mod, *internals);
  auto pointSampleTriangleGeometryFilter = BindFilter<PointSampleTriangleGeometryFilter>(mod, *internals);
  auto quickSurfaceMeshFilter = BindFilter<QuickSurfaceMeshFilter>(mod, *internals);
  auto rawBinaryReaderFilter = BindFilter<RawBinaryReaderFilter>(mod, *internals);
  auto regularGridSampleSurfaceMeshFilter = BindFilter<RegularGridSampleSurfaceMeshFilter>(mod, *internals);
  auto removeFlaggedFeaturesFilter = BindFilter<RemoveFlaggedFeaturesFilter>(mod, *internals);
  auto removeFlaggedVertices = BindFilter<RemoveFlaggedVertices>(mod, *internals);
  auto removeMinimumSizeFeaturesFilter = BindFilter<RemoveMinimumSizeFeaturesFilter>(mod, *internals);
  auto renameDataObject = BindFilter<RenameDataObject>(mod, *internals);
  auto replaceElementAttributesWithNeighborValuesFilter = BindFilter<ReplaceElementAttributesWithNeighborValuesFilter>(mod, *internals);
  auto resampleImageGeomFilter = BindFilter<ResampleImageGeomFilter>(mod, *internals);
  auto resampleRectGridToImageGeomFilter = BindFilter<ResampleRectGridToImageGeomFilter>(mod, *internals);
  auto robustAutomaticThreshold = BindFilter<RobustAutomaticThreshold>(mod, *internals);
  auto rotateSampleRefFrameFilter = BindFilter<RotateSampleRefFrameFilter>(mod, *internals);
  auto scalarSegmentFeaturesFilter = BindFilter<ScalarSegmentFeaturesFilter>(mod, *internals);
  auto setImageGeomOriginScalingFilter = BindFilter<SetImageGeomOriginScalingFilter>(mod, *internals);
  auto sharedFeatureFaceFilter = BindFilter<SharedFeatureFaceFilter>(mod, *internals);
  auto splitAttributeArrayFilter = BindFilter<SplitAttributeArrayFilter>(mod, *internals);
  auto stlFileReaderFilter = BindFilter<StlFileReaderFilter>(mod, *internals);
  auto triangleCentroidFilter = BindFilter<TriangleCentroidFilter>(mod, *internals);
  auto triangleDihedralAngleFilter = BindFilter<TriangleDihedralAngleFilter>(mod, *internals);
  auto triangleNormalFilter = BindFilter<TriangleNormalFilter>(mod, *internals);
  auto uncertainRegularGridSampleSurfaceMeshFilter = BindFilter<UncertainRegularGridSampleSurfaceMeshFilter>(mod, *internals);
  auto vtkRectilinearGridWriterFilter = BindFilter<VtkRectilinearGridWriterFilter>(mod, *internals);
  auto writeASCIIDataFilter = BindFilter<WriteASCIIDataFilter>(mod, *internals);
  auto writeBinaryDataFilter = BindFilter<WriteBinaryDataFilter>(mod, *internals);
  auto writeStlFileFilter = BindFilter<WriteStlFileFilter>(mod, *internals);

  internals->registerPluginPyFilters(*corePlugin);

  mod.def("get_all_data_types", &GetAllDataTypes);

  mod.def("convert_numeric_type_to_data_type", &ConvertNumericTypeToDataType);

  mod.def("get_filters", [internals, corePlugin]() { return internals->getPluginPyFilters(corePlugin->getId()); });

  mod.def("test_filter", [](const IFilter& filter) { return py::make_tuple(filter.uuid(), filter.name(), filter.humanName(), filter.className(), filter.defaultTags()); });

  mod.def("load_python_plugin", [internals](py::module_& mod) { internals->loadPythonPlugin(mod); });

  mod.def("get_python_plugins", [internals]() {
    auto plugins = internals->getPythonPlugins();
    std::vector<std::shared_ptr<AbstractPlugin>> abstractPlugins(plugins.cbegin(), plugins.cend());
    return abstractPlugins;
  });

  mod.def("get_python_filter_ids", [internals]() {
    auto plugins = internals->getPythonPlugins();
    std::vector<Uuid> ids;
    for(const auto& plugin : plugins)
    {
      auto handles = plugin->getFilterHandles();
      for(const auto& handle : handles)
      {
        ids.push_back(handle.getFilterId());
      }
    }
    return ids;
  });

  mod.def("reload_python_plugins", [internals]() { internals->reloadPythonPlugins(); });
}
