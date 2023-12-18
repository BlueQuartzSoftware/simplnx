#include "InitializeImageGeomCellData.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <array>
#include <chrono>
#include <limits>
#include <random>
#include <thread>

using namespace complex;

namespace
{
using RangeType = std::pair<float64, float64>;

InitializeImageGeomCellData::InitType ConvertIndexToInitType(uint64 index)
{
  switch(index)
  {
  case to_underlying(InitializeImageGeomCellData::InitType::Manual): {
    return InitializeImageGeomCellData::InitType::Manual;
  }
  case to_underlying(InitializeImageGeomCellData::InitType::Random): {
    return InitializeImageGeomCellData::InitType::Random;
  }
  case to_underlying(InitializeImageGeomCellData::InitType::RandomWithRange): {
    return InitializeImageGeomCellData::InitType::RandomWithRange;
  }
  default: {
    throw std::runtime_error("InitializeImageGeomCellData: Invalid value for InitType");
  }
  }
}

struct CheckInitializationFunctor
{
  template <class T>
  std::optional<Error> operator()(const IDataArray& dataArray, InitializeImageGeomCellData::InitType initType, float64 initValue, const std::pair<float64, float64>& initRange)
  {
    std::string arrayName = dataArray.getName();

    if(initType == InitializeImageGeomCellData::InitType::Manual)
    {
      if(initValue < static_cast<double>(std::numeric_limits<T>().lowest()) || initValue > static_cast<double>(std::numeric_limits<T>().max()))
      {
        return Error{-4000, fmt::format("{}: The initialization value could not be converted. The valid range is {} to {}", arrayName, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())};
      }
    }
    else if(initType == InitializeImageGeomCellData::InitType::RandomWithRange)
    {
      float64 min = initRange.first;
      float64 max = initRange.second;
      if(min > max)
      {
        return Error{-5550, fmt::format("{}: Invalid initialization range.  Minimum value is larger than maximum value.", arrayName)};
      }
      if(min < static_cast<double>(std::numeric_limits<T>().lowest()) || max > static_cast<double>(std::numeric_limits<T>().max()))
      {
        return Error{-4001, fmt::format("{}: The initialization range can only be from {} to {}", arrayName, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())};
      }
      if(min == max)
      {
        return Error{-4002, fmt::format("{}: The initialization range must have differing values", arrayName)};
      }
    }

    return {};
  }
};

template <class T>
auto CreateRandomGenerator(T rangeMin, T rangeMax, uint64 seed)
{
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  generator.seed(seed);

  if constexpr(std::is_integral_v<T>)
  {
    std::uniform_int_distribution<> distribution(rangeMin, rangeMax);
    return std::make_pair(distribution, generator);
  }
  else if constexpr(std::is_floating_point_v<T>)
  {
    std::uniform_real_distribution<T> distribution(rangeMin, rangeMax);
    return std::make_pair(distribution, generator);
  }
}

struct InitializeArrayFunctor
{
  template <class T>
  void operator()(IDataArray& dataArray, const std::array<usize, 3>& dims, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax, InitializeImageGeomCellData::InitType initType,
                  float64 initValue, const RangeType& initRange, uint64 seed)
  {
    T rangeMin;
    T rangeMax;
    if(initType == InitializeImageGeomCellData::InitType::RandomWithRange)
    {
      rangeMin = static_cast<T>(initRange.first);
      rangeMax = static_cast<T>(initRange.second);
    }
    else
    {
      rangeMin = std::numeric_limits<T>().min();
      rangeMax = std::numeric_limits<T>().max();
    }

    auto& dataStore = dataArray.getIDataStoreRefAs<AbstractDataStore<T>>();

    auto&& [distribution, generator] = CreateRandomGenerator(rangeMin, rangeMax, seed);

    for(uint64 k = zMin; k < zMax + 1; k++)
    {
      for(uint64 j = yMin; j < yMax + 1; j++)
      {
        for(uint64 i = xMin; i < xMax + 1; i++)
        {
          usize index = (k * dims[0] * dims[1]) + (j * dims[0]) + i;

          if(initType == InitializeImageGeomCellData::InitType::Manual)
          {
            T num = static_cast<T>(initValue);
            dataStore.fillTuple(index, num);
          }
          else
          {
            T randNum = distribution(generator);
            dataStore.fillTuple(index, randNum);
          }
        }
      }
    }
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string InitializeImageGeomCellData::name() const
{
  return FilterTraits<InitializeImageGeomCellData>::name;
}

//------------------------------------------------------------------------------
std::string InitializeImageGeomCellData::className() const
{
  return FilterTraits<InitializeImageGeomCellData>::className;
}

//------------------------------------------------------------------------------
Uuid InitializeImageGeomCellData::uuid() const
{
  return FilterTraits<InitializeImageGeomCellData>::uuid;
}

//------------------------------------------------------------------------------
std::string InitializeImageGeomCellData::humanName() const
{
  return "Initialize Image Geometry Cell Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> InitializeImageGeomCellData::defaultTags() const
{
  return {className(), "Memory Management", "Initialize", "Create", "Generate", "Data"};
}

//------------------------------------------------------------------------------
Parameters InitializeImageGeomCellData::parameters() const
{
  Parameters params;

  // TODO: restrict types
  params.insertSeparator(Parameters::Separator{"Seeded Randomness"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "InitializeImageGeomCellData SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MinPoint_Key, "Min Point", "The minimum x, y, z bound in cells", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MaxPoint_Key, "Max Point", "The maximum x, y, z bound in cells", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "Tells how to initialize the data", 0, ChoicesParameter::Choices{"Manual", "Random", "Random With Range"}));
  params.insert(std::make_unique<Float64Parameter>(k_InitValue_Key, "Initialization Value", "The initialization value if Manual Initialization Type is selected", 0.0f));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_InitRange_Key, "Initialization Range", "The initialization range if Random With Range Initialization Type is selected",
                                                         VectorFloat64Parameter::ValueType{0.0, 0.0}));
  params.linkParameters(k_InitType_Key, k_InitValue_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_InitType_Key, k_InitRange_Key, std::make_any<ChoicesParameter::ValueType>(2));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CellArrayPaths_Key, "Cell Arrays", "The cell data arrays in which to initialize a sub-volume to zeros", std::vector<DataPath>{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllDataTypes()));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The geometry containing the cell data for which to initialize", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InitializeImageGeomCellData::clone() const
{
  return std::make_unique<InitializeImageGeomCellData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InitializeImageGeomCellData::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto cellArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeometryPath_Key);
  auto minPoint = args.value<std::vector<uint64>>(k_MinPoint_Key);
  auto maxPoint = args.value<std::vector<uint64>>(k_MaxPoint_Key);
  auto initTypeIndex = args.value<uint64>(k_InitType_Key);
  auto initValue = args.value<float64>(k_InitValue_Key);
  auto initRangeVec = args.value<std::vector<float64>>(k_InitRange_Key);
  auto pSeedArrayNameValue = args.value<std::string>(k_SeedArrayName_Key);

  uint64 xMin = minPoint.at(0);
  uint64 yMin = minPoint.at(1);
  uint64 zMin = minPoint.at(2);

  uint64 xMax = maxPoint.at(0);
  uint64 yMax = maxPoint.at(1);
  uint64 zMax = maxPoint.at(2);

  InitType initType = ConvertIndexToInitType(initTypeIndex);
  RangeType initRange = {initRangeVec.at(0), initRangeVec.at(1)};

  if(cellArrayPaths.empty())
  {
    return {MakeErrorResult<OutputActions>(-5550, "At least one data array must be selected.")};
  }

  std::vector<Error> errors;

  if(xMax < xMin)
  {
    errors.push_back(Error{-5551, fmt::format("X Max ({}) less than X Min ({})", xMax, xMin)});
  }
  if(yMax < yMin)
  {
    errors.push_back(Error{-5552, fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin)});
  }
  if(zMax < zMin)
  {
    errors.push_back(Error{-5553, fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin)});
  }

  const auto& imageGeom = data.getDataRefAs<ImageGeom>(imageGeomPath);

  if(xMax > (static_cast<int64>(imageGeom.getNumXCells()) - 1))
  {
    errors.push_back(Error{-5557, fmt::format("The X Max you entered of {} is greater than your Max X Point of {}", xMax, static_cast<int64>(imageGeom.getNumXCells()) - 1)});
  }
  if(yMax > (static_cast<int64>(imageGeom.getNumYCells()) - 1))
  {
    errors.push_back(Error{-5558, fmt::format("The Y Max you entered of {} is greater than your Max Y Point of {}", yMax, static_cast<int64>(imageGeom.getNumYCells()) - 1)});
  }
  if(zMax > (static_cast<int64>(imageGeom.getNumZCells()) - 1))
  {
    errors.push_back(Error{-5559, fmt::format("The Z Max you entered of {} is greater than your Max Z Point of {}", zMax, static_cast<int64>(imageGeom.getNumZCells()) - 1)});
  }

  SizeVec3 imageDims = imageGeom.getDimensions();

  std::vector<usize> reversedImageDims(std::make_reverse_iterator(imageDims.end()), std::make_reverse_iterator(imageDims.begin()));

  for(const DataPath& path : cellArrayPaths)
  {
    const auto& dataArray = data.getDataRefAs<IDataArray>(path);
    std::vector<usize> tupleShape = dataArray.getIDataStoreRef().getTupleShape();

    if(tupleShape.size() != reversedImageDims.size())
    {
      errors.push_back(Error{-5560, fmt::format("DataArray at '{}' does not match dimensions of ImageGeometry at '{}'", path.toString(), imageGeomPath.toString())});
      continue;
    }

    std::optional<Error> maybeError = ExecuteNeighborFunction(CheckInitializationFunctor{}, dataArray.getDataType(), dataArray, initType, initValue, initRange); // NO BOOL
    if(maybeError.has_value())
    {
      errors.push_back(std::move(*maybeError));
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  complex::Result<OutputActions> resultOutputActions;

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> InitializeImageGeomCellData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto cellArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeometryPath_Key);
  auto minPoint = args.value<std::vector<uint64>>(k_MinPoint_Key);
  auto maxPoint = args.value<std::vector<uint64>>(k_MaxPoint_Key);
  auto initTypeIndex = args.value<uint64>(k_InitType_Key);
  auto initValue = args.value<float64>(k_InitValue_Key);
  auto initRangeVec = args.value<std::vector<float64>>(k_InitRange_Key);

  auto seed = args.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!args.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  data.getDataRefAs<UInt64Array>(DataPath({args.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  uint64 xMin = minPoint.at(0);
  uint64 yMin = minPoint.at(1);
  uint64 zMin = minPoint.at(2);

  uint64 xMax = maxPoint.at(0);
  uint64 yMax = maxPoint.at(1);
  uint64 zMax = maxPoint.at(2);

  InitType initType = ConvertIndexToInitType(initTypeIndex);
  RangeType initRange = {initRangeVec.at(0), initRangeVec.at(1)};

  const ImageGeom& imageGeom = data.getDataRefAs<ImageGeom>(imageGeomPath);

  std::array<usize, 3> dims = imageGeom.getDimensions().toArray();

  for(const DataPath& path : cellArrayPaths)
  {
    auto& iDataArray = data.getDataRefAs<IDataArray>(path);

    ExecuteNeighborFunction(InitializeArrayFunctor{}, iDataArray.getDataType(), iDataArray, dims, xMin, xMax, yMin, yMax, zMin, zMax, initType, initValue, initRange, seed); // NO BOOL

    // Avoid the exact same seeding for each array
    seed++;
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CellAttributeMatrixPathsKey = "CellAttributeMatrixPaths";
constexpr StringLiteral k_XMinKey = "XMin";
constexpr StringLiteral k_YMinKey = "YMin";
constexpr StringLiteral k_ZMinKey = "ZMin";
constexpr StringLiteral k_XMaxKey = "XMax";
constexpr StringLiteral k_YMaxKey = "YMax";
constexpr StringLiteral k_ZMaxKey = "ZMax";
constexpr StringLiteral k_InitTypeKey = "InitType";
constexpr StringLiteral k_InitValueKey = "InitValue";
constexpr StringLiteral k_InitRangeKey = "InitRange";
constexpr StringLiteral k_InvertDataKey = "InvertData";
} // namespace SIMPL
} // namespace

Result<Arguments> InitializeImageGeomCellData::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = InitializeImageGeomCellData().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerFromMultiSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathsKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathsKey, k_CellArrayPaths_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMinKey, SIMPL::k_YMinKey, SIMPL::k_ZMinKey, k_MinPoint_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMaxKey, SIMPL::k_YMaxKey, SIMPL::k_ZMaxKey, k_MaxPoint_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_InitTypeKey, k_InitType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_InitValueKey, k_InitValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::RangeFilterParameterConverter>(args, json, SIMPL::k_InitRangeKey, k_InitRange_Key));
  // Invert Data parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
