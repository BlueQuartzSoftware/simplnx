#include "CreateDataArrayFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataStoreFormatParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <stdexcept>

using namespace nx::core;

namespace
{
constexpr int32 k_EmptyParameterError = -123;

struct CreateAndInitArrayFunctor
{
  template <class T>
  void operator()(IDataArray* iDataArray, const std::string& initValue)
  {
    Result<T> result = ConvertTo<T>::convert(initValue);

    auto* dataStore = iDataArray->template getIDataStoreAs<AbstractDataStore<T>>();
    dataStore->fill(result.value());
  }
};
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateDataArrayFilter::name() const
{
  return FilterTraits<CreateDataArrayFilter>::name;
}

//------------------------------------------------------------------------------
std::string CreateDataArrayFilter::className() const
{
  return FilterTraits<CreateDataArrayFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateDataArrayFilter::uuid() const
{
  return FilterTraits<CreateDataArrayFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateDataArrayFilter::humanName() const
{
  return "Create Data Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateDataArrayFilter::defaultTags() const
{
  return {className(), "Create", "Data Structure", "Data Array", "Initialize", "Make"};
}

//------------------------------------------------------------------------------
Parameters CreateDataArrayFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataPath_Key, "Created Array", "Array storing the data", DataPath({"Data"})));
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Output Numeric Type", "Numeric Type of data to create", NumericType::int32));
  params.insert(std::make_unique<DataStoreFormatParameter>(k_DataFormat_Key, "Data Format",
                                                           "This value will specify which data format is used by the array's data store. An empty string results in in-memory data store.", ""));

  params.insertSeparator(Parameters::Separator{"Tuple Dimensions"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_AdvancedOptions_Key, "Set Tuple Dimensions [not required if creating inside an Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix", true));
  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "DIM {}"));

  params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Data Array Dimensions (Slowest to Fastest Dimensions)",
                                                        "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", tableInfo));

  params.insertSeparator(Parameters::Separator{"Component Dimensions"});
  params.insert(std::make_unique<UInt64Parameter>(k_NumComps_Key, "Total Number of Components", "Total number of components. Do not set the component dimensions.", 1));

  params.insertSeparator(Parameters::Separator{"Initialization Options"});
  params.insert(std::make_unique<StringParameter>(k_InitializationValue_Key, "Initialization Value", "This value will be used to fill the new array", "0"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_AdvancedOptions_Key, k_TupleDims_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CreateDataArrayFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateDataArrayFilter::clone() const
{
  return std::make_unique<CreateDataArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateDataArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto useDims = filterArgs.value<bool>(k_AdvancedOptions_Key);
  auto numericType = filterArgs.value<NumericType>(k_NumericType_Key);
  auto numComponents = filterArgs.value<uint64>(k_NumComps_Key);
  auto dataArrayPath = filterArgs.value<DataPath>(k_DataPath_Key);
  auto initValue = filterArgs.value<std::string>(k_InitializationValue_Key);
  auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);
  auto dataFormat = filterArgs.value<std::string>(k_DataFormat_Key);

  nx::core::Result<OutputActions> resultOutputActions;

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
Result<> CreateDataArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto path = args.value<DataPath>(k_DataPath_Key);
  auto initValue = args.value<std::string>(k_InitializationValue_Key);

  ExecuteNeighborFunction(CreateAndInitArrayFunctor{}, ConvertNumericTypeToDataType(args.value<NumericType>(k_NumericType_Key)), dataStructure.getDataAs<IDataArray>(path), initValue);

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ScalarTypeKey = "ScalarType";
constexpr StringLiteral k_NumberOfComponentsKey = "NumberOfComponents";
constexpr StringLiteral k_InitializationValueKey = "InitializationValue";
constexpr StringLiteral k_NewArrayKey = "NewArray";
} // namespace SIMPL
} // namespace

Result<Arguments> CreateDataArrayFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateDataArrayFilter().getDefaultArguments();

  args.insertOrAssign(k_AdvancedOptions_Key, false);

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ScalarTypeParameterToNumericTypeConverter>(args, json, SIMPL::k_ScalarTypeKey, k_NumericType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_NumberOfComponentsKey, k_NumComps_Key));
  // Initialize Type parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_InitializationValueKey, k_InitializationValue_Key));
  // Initialization Range parameter is not applicable in NX
  // Starting Index value parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationFilterParameterConverter>(args, json, SIMPL::k_NewArrayKey, k_DataPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
