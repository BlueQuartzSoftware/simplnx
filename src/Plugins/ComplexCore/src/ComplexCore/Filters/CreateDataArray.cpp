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

template <typename T>
struct ULLConvertor
{
  T operator()(const std::string& initValue)
  {
    return static_cast<T>(std::stoull(initValue));
  }
};

struct Float32Convertor
{
  complex::float32 operator()(const std::string& initValue)
  {
    return static_cast<complex::float32>(std::stof(initValue));
  }
};

struct Float64Convertor
{
  complex::float64 operator()(const std::string& initValue)
  {
    return static_cast<complex::float64>(std::stod(initValue));
  }
};

template <typename T, class Convertor>
void CreateAndInitArray(const std::string& initValue, complex::DataStructure& data, const complex::DataPath& path)
{
  T value = static_cast<T>(Convertor()(initValue));
  auto& dataArray = complex::ArrayRefFromPath<T>(data, path);
  auto& v = *(dataArray.getDataStore());
  std::fill(v.begin(), v.end(), value);
}

} // namespace

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
  auto path = args.value<DataPath>(k_DataPath_Key);
  auto initValue = args.value<std::string>(k_InitilizationValue_Key);

  switch(numericType)
  {
  case NumericType::int8: {
    CreateAndInitArray<int8, ULLConvertor<int8>>(initValue, data, path);
    break;
  }
  case NumericType::uint8: {
    CreateAndInitArray<uint8, ULLConvertor<uint8>>(initValue, data, path);
    break;
  }
  case NumericType::int16: {
    CreateAndInitArray<int16, ULLConvertor<int16>>(initValue, data, path);
    break;
  }
  case NumericType::uint16: {
    CreateAndInitArray<uint16, ULLConvertor<uint16>>(initValue, data, path);
    break;
  }
  case NumericType::int32: {
    CreateAndInitArray<int32, ULLConvertor<int32>>(initValue, data, path);
    break;
  }
  case NumericType::uint32: {
    CreateAndInitArray<uint32, ULLConvertor<uint32>>(initValue, data, path);
    break;
  }
  case NumericType::int64: {
    CreateAndInitArray<int64, ULLConvertor<int64>>(initValue, data, path);
    break;
  }
  case NumericType::uint64: {
    CreateAndInitArray<uint64, ULLConvertor<uint64>>(initValue, data, path);
    break;
  }
  case NumericType::float32: {
    CreateAndInitArray<float32, Float32Convertor>(initValue, data, path);
    break;
  }
  case NumericType::float64: {
    CreateAndInitArray<float64, Float64Convertor>(initValue, data, path);
    break;
  }
  default:
    return MakeErrorResult(-10103, fmt::format("Invalid NumericType used"));
  }

  return {};
}
} // namespace complex
