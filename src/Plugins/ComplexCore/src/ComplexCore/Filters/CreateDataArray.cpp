#include "CreateDataArray.hpp"

#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <string>

namespace
{
constexpr complex::int32 k_EMPTY_PARAMETER = -123;
}

namespace complex
{

std::string CreateDataArray::name() const
{
  return FilterTraits<CreateDataArray>::name;
}

std::string CreateDataArray::className() const
{
  return FilterTraits<CreateDataArray>::className;
}

Uuid CreateDataArray::uuid() const
{
  return FilterTraits<CreateDataArray>::uuid;
}

std::string CreateDataArray::humanName() const
{
  return "Create Data Array";
}

Parameters CreateDataArray::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Numeric Type", "Numeric Type of data to create", NumericType::int32));
  params.insert(std::make_unique<UInt64Parameter>(k_NumComps_Key, "Number of Components", "Number of components", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NumTuples_Key, "Number of Tuples", "Number of tuples", 0));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key, "Created Array", "Array storing the data", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_InitilizationValue_Key, "Initialization Value", "This value will be used to fill the new array", "0"));
  return params;
}

IFilter::UniquePointer CreateDataArray::clone() const
{
  return std::make_unique<CreateDataArray>();
}

IFilter::PreflightResult CreateDataArray::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto numericType = filterArgs.value<NumericType>(k_NumericType_Key);
  auto components = filterArgs.value<uint64>(k_NumComps_Key);
  auto numTuples = filterArgs.value<uint64>(k_NumTuples_Key);
  auto dataArrayPath = filterArgs.value<DataPath>(k_DataPath_Key);
  auto initValue = filterArgs.value<std::string>(k_InitilizationValue_Key);

  if(initValue.empty())
  {
    return {MakeErrorResult<OutputActions>(::k_EMPTY_PARAMETER, fmt::format("{}: Init Value cannot be empty.{}({})", humanName(), __FILE__, __LINE__)), {}};
  }
  // Create the Output Array though an action. NOTE: The data array will NOT be immediately available to this function.
  auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<usize>{numTuples}, std::vector<usize>{components}, dataArrayPath);
  // Sanity check that what the user entered for an init value can be converted safely to the final numeric type
  Result<> result = complex::CheckInitValueConverts(initValue, numericType);
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(result), {})};
  }

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CreateDataArray::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key);
  auto components = args.value<uint64>(k_NumComps_Key);
  auto numTuples = args.value<uint64>(k_NumTuples_Key);
  auto path = args.value<DataPath>(k_DataPath_Key);
  auto initValue = args.value<std::string>(k_InitilizationValue_Key);

  switch(numericType)
  {
  case NumericType::int8: {
    int8_t value = static_cast<int8>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<int8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::uint8: {
    uint8 value = static_cast<uint8>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<uint8>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::int16: {
    int16 value = static_cast<int16>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<int16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::uint16: {
    uint16 value = static_cast<uint16>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<uint16>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::int32: {
    int32 value = static_cast<int32>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<int32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::uint32: {
    uint32 value = static_cast<uint32>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<uint32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::int64: {
    int64 value = static_cast<int64>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<int64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::uint64: {
    uint64 value = static_cast<uint64>(std::stoull(initValue));
    auto& dataArray = ArrayRefFromPath<uint64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::float32: {
    float32 value = static_cast<float32>(std::stof(initValue));
    auto& dataArray = ArrayRefFromPath<float32>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  case NumericType::float64: {
    float64 value = static_cast<float64>(std::stod(initValue));
    auto& dataArray = ArrayRefFromPath<float64>(data, path);
    auto& v = *(dataArray.getDataStore());
    std::fill(v.begin(), v.end(), value);
    break;
  }
  default:
    return MakeErrorResult(-10103, fmt::format("Invalid NumericType used"));
  }

  return {};
}
} // namespace complex
