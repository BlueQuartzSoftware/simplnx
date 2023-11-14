#include "CreateOutOfCoreArray.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <stdexcept>

using namespace complex;

namespace
{
constexpr int32 k_EmptyParameterError = -123;
constexpr int32 k_RbrTupleDimsError = -196;
constexpr int32 k_RbrTupleDimsInconsistent = -197;

template <class T>
void CreateAndInitArray(DataStructure& data, const DataPath& path, const std::string& initValue)
{
  Result<T> result = ConvertTo<T>::convert(initValue);
  T value = result.value();
  auto& dataArray = data.getDataRefAs<DataArray<T>>(path);
  auto& dataStore = dataArray.getDataStoreRef();
  std::fill(dataStore.begin(), dataStore.end(), value);
}
} // namespace

namespace complex
{
std::string CreateOutOfCoreArray::name() const
{
  return FilterTraits<CreateOutOfCoreArray>::name;
}

std::string CreateOutOfCoreArray::className() const
{
  return FilterTraits<CreateOutOfCoreArray>::className;
}

Uuid CreateOutOfCoreArray::uuid() const
{
  return FilterTraits<CreateOutOfCoreArray>::uuid;
}

std::string CreateOutOfCoreArray::humanName() const
{
  return "Create Out-Of-Core Data Array";
}

Parameters CreateOutOfCoreArray::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Output Numeric Type", "Numeric Type of data to create", NumericType::int32));
  params.insert(std::make_unique<UInt64Parameter>(k_NumComps_Key, "Number of Components", "Number of components", 1));
  DynamicTableParameter::ValueType dynamicTable(1, 1, {"DIM"}, {""});
  dynamicTable.setMinCols(1);
  dynamicTable.setDynamicCols(true);
  dynamicTable.setDynamicRows(false);
  params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Data Array Dimensions (Slowest to Fastest Dimensions)", "Slowest to Fastest Dimensions", dynamicTable));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key, "Created Array", "Array storing the data", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_InitilizationValue_Key, "Initialization Value", "This value will be used to fill the new array", "0"));
  return params;
}

IFilter::UniquePointer CreateOutOfCoreArray::clone() const
{
  return std::make_unique<CreateOutOfCoreArray>();
}

IFilter::PreflightResult CreateOutOfCoreArray::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto numericType = filterArgs.value<NumericType>(k_NumericType_Key);
  auto numComponents = filterArgs.value<uint64>(k_NumComps_Key);
  // auto numTuples = filterArgs.value<uint64>(k_NumTuples_Key);
  auto dataArrayPath = filterArgs.value<DataPath>(k_DataPath_Key);
  auto initValue = filterArgs.value<std::string>(k_InitilizationValue_Key);
  auto pTupleDimsValue = filterArgs.value<DynamicTableData>(k_TupleDims_Key);

  if(initValue.empty())
  {
    return {MakeErrorResult<OutputActions>(k_EmptyParameterError, fmt::format("{}: Init Value cannot be empty.{}({})", humanName(), __FILE__, __LINE__)), {}};
  }
  // Sanity check that what the user entered for an init value can be converted safely to the final numeric type
  Result<> result = CheckValueConverts(initValue, numericType);
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(result), {})};
  }

  // Sanity check the values from the Dynamic Table Data
  DynamicTableData::TableDataType tableData = pTupleDimsValue.getTableData();
  if(tableData.size() != 1)
  {
    return {MakeErrorResult<OutputActions>(k_RbrTupleDimsError, fmt::format("Tuple Dimensions should be a single row of data. {} Rows were passed.", tableData.size()))};
  }
  const std::vector<DynamicTableData::DataType>& rowData = tableData[0];
  std::vector<size_t> tupleDims;
  tupleDims.reserve(rowData.size()); // Reserve (NOT RESIZE) the proper number of values in the vector
  for(const auto& floatValue : rowData)
  {
    tupleDims.push_back(static_cast<size_t>(floatValue));
  }

  OutputActions actions;
  auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(numericType), tupleDims, std::vector<usize>{numComponents}, dataArrayPath, false);
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

Result<> CreateOutOfCoreArray::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto numericType = args.value<NumericType>(k_NumericType_Key);
  auto path = args.value<DataPath>(k_DataPath_Key);
  auto initValue = args.value<std::string>(k_InitilizationValue_Key);

  switch(numericType)
  {
  case NumericType::int8: {
    CreateAndInitArray<int8>(data, path, initValue);
    break;
  }
  case NumericType::uint8: {
    CreateAndInitArray<uint8>(data, path, initValue);
    break;
  }
  case NumericType::int16: {
    CreateAndInitArray<int16>(data, path, initValue);
    break;
  }
  case NumericType::uint16: {
    CreateAndInitArray<uint16>(data, path, initValue);
    break;
  }
  case NumericType::int32: {
    CreateAndInitArray<int32>(data, path, initValue);
    break;
  }
  case NumericType::uint32: {
    CreateAndInitArray<uint32>(data, path, initValue);
    break;
  }
  case NumericType::int64: {
    CreateAndInitArray<int64>(data, path, initValue);
    break;
  }
  case NumericType::uint64: {
    CreateAndInitArray<uint64>(data, path, initValue);
    break;
  }
  case NumericType::float32: {
    CreateAndInitArray<float32>(data, path, initValue);
    break;
  }
  case NumericType::float64: {
    CreateAndInitArray<float64>(data, path, initValue);
    break;
  }
  default:
    throw std::runtime_error("Invalid NumericType used");
  }

  return {};
}
} // namespace complex
