#include "InitializeData.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

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

InitializeData::InitType ConvertIndexToInitType(uint64 index)
{
  switch(index)
  {
  case to_underlying(InitializeData::InitType::Manual): {
    return InitializeData::InitType::Manual;
  }
  case to_underlying(InitializeData::InitType::Random): {
    return InitializeData::InitType::Random;
  }
  case to_underlying(InitializeData::InitType::RandomWithRange): {
    return InitializeData::InitType::RandomWithRange;
  }
  default: {
    throw std::runtime_error("InitializeData: Invalid value for InitType");
  }
  }
}

template <class T>
std::optional<Error> CheckInitialization(const IDataArray& dataArray, InitializeData::InitType initType, float64 initValue, const std::pair<float64, float64>& initRange)
{
  std::string arrayName = dataArray.getName();

  if(initType == InitializeData::InitType::Manual)
  {
    if(initValue < static_cast<double>(std::numeric_limits<T>().lowest()) || initValue > static_cast<double>(std::numeric_limits<T>().max()))
    {
      return Error{-4000, fmt::format("{}: The initialization value could not be converted. The valid range is {} to {}", arrayName, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())};
    }
  }
  else if(initType == InitializeData::InitType::RandomWithRange)
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

template <class T>
auto CreateRandomGenerator(T rangeMin, T rangeMax)
{
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  std::mt19937_64::result_type seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
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

template <class T>
void InitializeArray(IDataArray& dataArray, const std::array<usize, 3>& dims, uint64 xMin, uint64 xMax, uint64 yMin, uint64 yMax, uint64 zMin, uint64 zMax, InitializeData::InitType initType,
                     float64 initValue, const RangeType& initRange)
{
  T rangeMin;
  T rangeMax;
  if(initType == InitializeData::InitType::RandomWithRange)
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

  auto&& [distribution, generator] = CreateRandomGenerator(rangeMin, rangeMax);

  for(uint64 k = zMin; k < zMax + 1; k++)
  {
    for(uint64 j = yMin; j < yMax + 1; j++)
    {
      for(uint64 i = xMin; i < xMax + 1; i++)
      {
        usize index = (k * dims[0] * dims[1]) + (j * dims[0]) + i;

        if(initType == InitializeData::InitType::Manual)
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
} // namespace

namespace complex
{
std::string InitializeData::name() const
{
  return FilterTraits<InitializeData>::name;
}

std::string InitializeData::className() const
{
  return FilterTraits<InitializeData>::className;
}

Uuid InitializeData::uuid() const
{
  return FilterTraits<InitializeData>::uuid;
}

std::string InitializeData::humanName() const
{
  return "Initialize Data";
}

Parameters InitializeData::parameters() const
{
  Parameters params;
  // TODO: restrict types
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CellArrayPaths_Key, "Cell Arrays", "", std::vector<DataPath>{}, complex::GetAllDataTypes()));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MinPoint_Key, "Min Point", "", std::vector<uint64>{0, 0, 0}, std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MaxPoint_Key, "Max Point", "", std::vector<uint64>{0, 0, 0}, std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "", 0, ChoicesParameter::Choices{"Manual", "Random", "Random With Range"}));
  params.insert(std::make_unique<Float64Parameter>(k_InitValue_Key, "Initialization Value", "", 0.0f));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_InitRange_Key, "Initialization Range", "", VectorFloat64Parameter::ValueType{0.0, 0.0}));
  params.linkParameters(k_InitType_Key, k_InitValue_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_InitType_Key, k_InitRange_Key, std::make_any<ChoicesParameter::ValueType>(2));
  return params;
}

IFilter::UniquePointer InitializeData::clone() const
{
  return std::make_unique<InitializeData>();
}

IFilter::PreflightResult InitializeData::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto cellArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeometryPath_Key);
  auto minPoint = args.value<std::vector<uint64>>(k_MinPoint_Key);
  auto maxPoint = args.value<std::vector<uint64>>(k_MaxPoint_Key);
  auto initTypeIndex = args.value<uint64>(k_InitType_Key);
  auto initValue = args.value<float64>(k_InitValue_Key);
  auto initRangeVec = args.value<std::vector<float64>>(k_InitRange_Key);

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

  const ImageGeom& imageGeom = data.getDataRefAs<ImageGeom>(imageGeomPath);

  if(xMax > (static_cast<int64>(imageGeom.getNumXPoints()) - 1))
  {
    errors.push_back(Error{-5557, fmt::format("The X Max you entered of {} is greater than your Max X Point of {}", xMax, static_cast<int64>(imageGeom.getNumXPoints()) - 1)});
  }
  if(yMax > (static_cast<int64>(imageGeom.getNumYPoints()) - 1))
  {
    errors.push_back(Error{-5558, fmt::format("The Y Max you entered of {} is greater than your Max Y Point of {}", yMax, static_cast<int64>(imageGeom.getNumYPoints()) - 1)});
  }
  if(zMax > (static_cast<int64>(imageGeom.getNumZPoints()) - 1))
  {
    errors.push_back(Error{-5559, fmt::format("The Z Max you entered of {} is greater than your Max Z Point of {}", zMax, static_cast<int64>(imageGeom.getNumZPoints()) - 1)});
  }

  SizeVec3 imageDims = imageGeom.getDimensions();

  std::vector<usize> reversedImageDims(std::make_reverse_iterator(imageDims.end()), std::make_reverse_iterator(imageDims.begin()));

  for(const DataPath& path : cellArrayPaths)
  {
    const IDataArray& dataArray = data.getDataRefAs<IDataArray>(path);

    std::vector<usize> tupleShape = dataArray.getIDataStoreRef().getTupleShape();

    if(tupleShape.size() != reversedImageDims.size())
    {
      errors.push_back(Error{-5560, fmt::format("DataArray at '{}' does not match dimensions of ImageGeometry at '{}'", path.toString(), imageGeomPath.toString())});
      continue;
    }

    DataType type = dataArray.getDataType();

    switch(type)
    {
    case DataType::int8: {
      std::optional<Error> maybeError = CheckInitialization<int8>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::int16: {
      std::optional<Error> maybeError = CheckInitialization<int16>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::int32: {
      std::optional<Error> maybeError = CheckInitialization<int32>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::int64: {
      std::optional<Error> maybeError = CheckInitialization<int64>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::uint8: {
      std::optional<Error> maybeError = CheckInitialization<uint8>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::uint16: {
      std::optional<Error> maybeError = CheckInitialization<uint16>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::uint32: {
      std::optional<Error> maybeError = CheckInitialization<uint32>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::uint64: {
      std::optional<Error> maybeError = CheckInitialization<uint64>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::float32: {
      std::optional<Error> maybeError = CheckInitialization<float32>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    case DataType::float64: {
      std::optional<Error> maybeError = CheckInitialization<float64>(dataArray, initType, initValue, initRange);
      if(maybeError.has_value())
      {
        errors.push_back(std::move(*maybeError));
      }
      break;
    }
    default: {
      errors.push_back(Error{-5560, fmt::format("'{}' is an invalid type", path.toString())});
      break;
    }
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}

Result<> InitializeData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto cellArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  auto imageGeomPath = args.value<DataPath>(k_ImageGeometryPath_Key);
  auto minPoint = args.value<std::vector<uint64>>(k_MinPoint_Key);
  auto maxPoint = args.value<std::vector<uint64>>(k_MaxPoint_Key);
  auto initTypeIndex = args.value<uint64>(k_InitType_Key);
  auto initValue = args.value<float64>(k_InitValue_Key);
  auto initRangeVec = args.value<std::vector<float64>>(k_InitRange_Key);

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
    IDataArray& dataArray = data.getDataRefAs<IDataArray>(path);

    DataType type = dataArray.getDataType();

    switch(type)
    {
    case DataType::int8: {
      InitializeArray<int8>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::int16: {
      InitializeArray<int16>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::int32: {
      InitializeArray<int32>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::int64: {
      InitializeArray<int64>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::uint8: {
      InitializeArray<uint8>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::uint16: {
      InitializeArray<uint16>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::uint32: {
      InitializeArray<uint32>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::uint64: {
      InitializeArray<uint64>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::float32: {
      InitializeArray<float32>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    case DataType::float64: {
      InitializeArray<float32>(dataArray, dims, xMin, xMax, yMin, yMax, zMin, zMin, initType, initValue, initRange);
      break;
    }
    default: {
      throw std::runtime_error(fmt::format("InitializeData: Invalid array type for '{}'", path.toString()));
    }
    }

    // Delay the execution by 1 second to avoid the exact same seedings for each array
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
    }
  }

  return {};
}
} // namespace complex
