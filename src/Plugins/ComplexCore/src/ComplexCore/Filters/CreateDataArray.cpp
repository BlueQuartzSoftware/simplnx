#include "CreateDataArray.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataStoreFormatParameter.hpp"
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

template <class T>
void CreateAndInitArray(DataStructure& data, const DataPath& path, const std::string& initValue)
{
  Result<T> result = ConvertTo<T>::convert(initValue);
  T value = result.value();
  auto& dataArray = data.getDataRefAs<DataArray<T>>(path);
  auto& dataStore = dataArray.getDataStoreRef();
  dataStore.fill(value);
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateDataArray::name() const
{
  return FilterTraits<CreateDataArray>::name;
}

//------------------------------------------------------------------------------
std::string CreateDataArray::className() const
{
  return FilterTraits<CreateDataArray>::className;
}

//------------------------------------------------------------------------------
Uuid CreateDataArray::uuid() const
{
  return FilterTraits<CreateDataArray>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateDataArray::humanName() const
{
  return "Create Data Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateDataArray::defaultTags() const
{
  return {className(), "Create", "Data Structure", "Data Array", "Initialize", "Make"};
}

//------------------------------------------------------------------------------
Parameters CreateDataArray::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Output Numeric Type", "Numeric Type of data to create", NumericType::int32));
  params.insert(std::make_unique<StringParameter>(k_InitilizationValue_Key, "Initialization Value", "This value will be used to fill the new array", "0"));
  params.insert(std::make_unique<UInt64Parameter>(k_NumComps_Key, "Number of Components", "Number of components", 1));

  params.insertSeparator(Parameters::Separator{"Created DataArray"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key, "Created Array", "Array storing the data", DataPath{}));
  params.insert(std::make_unique<DataStoreFormatParameter>(k_DataFormat_Key, "Data Format",
                                                           "This value will specify which data format is used by the array's data store. An empty string results in in-memory data store.", ""));

  params.insertSeparator(Parameters::Separator{"Tuple Handling"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_AdvancedOptions_Key, "Set Tuple Dimensions [not required if creating inside an Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix", true));
  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "DIM {}"));

  params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Data Array Dimensions (Slowest to Fastest Dimensions)",
                                                        "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", tableInfo));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_AdvancedOptions_Key, k_TupleDims_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateDataArray::clone() const
{
  return std::make_unique<CreateDataArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateDataArray::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto useDims = filterArgs.value<bool>(k_AdvancedOptions_Key);
  auto numericType = filterArgs.value<NumericType>(k_NumericType_Key);
  auto numComponents = filterArgs.value<uint64>(k_NumComps_Key);
  auto dataArrayPath = filterArgs.value<DataPath>(k_DataPath_Key);
  auto initValue = filterArgs.value<std::string>(k_InitilizationValue_Key);
  auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);
  auto dataFormat = filterArgs.value<std::string>(k_DataFormat_Key);

  complex::Result<OutputActions> resultOutputActions;

  if(initValue.empty())
  {
    return MakePreflightErrorResult(k_EmptyParameterError, fmt::format("{}: Init Value cannot be empty.{}({})", humanName(), __FILE__, __LINE__));
  }
  // Sanity check that what the user entered for an init value can be converted safely to the final numeric type
  Result<> result = CheckValueConverts(initValue, numericType);
  if(result.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(result), {})};
  }

  std::vector<usize> compDims = std::vector<usize>{numComponents};
  std::vector<usize> tupleDims = {};

  auto* parentAM = dataStructure.getDataAs<AttributeMatrix>(dataArrayPath.getParent());
  if(parentAM == nullptr)
  {
    if(!useDims)
    {
      return MakePreflightErrorResult(
          -78602, fmt::format("The DataArray to be created '{}'is not within an AttributeMatrix, so the dimensions cannot be determined implicitly. Check Set Tuple Dimensions to set the dimensions",
                              dataArrayPath.toString()));
    }
    else
    {
      const auto& rowData = tableData.at(0);
      tupleDims.reserve(rowData.size());
      for(auto floatValue : rowData)
      {
        if(floatValue == 0)
        {
          return MakePreflightErrorResult(-78603, "Tuple dimension cannot be zero");
        }

        tupleDims.push_back(static_cast<usize>(floatValue));
      }
    }
  }
  else
  {
    tupleDims = parentAM->getShape();
    if(useDims)
    {
      resultOutputActions.warnings().push_back(
          Warning{-78604, "You checked Set Tuple Dimensions, but selected a DataPath that has an Attribute Matrix as the parent. The Attribute Matrix tuples will override your "
                          "custom dimensions. It is recommended to uncheck Set Tuple Dimensions for the sake of clarity."});
    }
  }

  auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(numericType), tupleDims, compDims, dataArrayPath, dataFormat);

  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> CreateDataArray::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
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
