#include "ReadTextDataArrayFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/ReadTextDataArray.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataStoreFormatParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
std::string ReadTextDataArrayFilter::name() const
{
  return FilterTraits<ReadTextDataArrayFilter>::name;
}

std::string ReadTextDataArrayFilter::className() const
{
  return FilterTraits<ReadTextDataArrayFilter>::className;
}

Uuid ReadTextDataArrayFilter::uuid() const
{
  return FilterTraits<ReadTextDataArrayFilter>::uuid;
}

std::vector<std::string> ReadTextDataArrayFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Text", "ASCII", "Attribute"};
}

std::string ReadTextDataArrayFilter::humanName() const
{
  return "Read Text Data Array";
}

Parameters ReadTextDataArrayFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "File path that points to the imported file", fs::path("<file to import goes here>"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Input Numeric Type", "Data Type to interpret and store data into.", NumericType::int8));
  params.insert(std::make_unique<UInt64Parameter>(k_NComp_Key, "Number of Components", "Number of columns", 1));
  params.insert(std::make_unique<UInt64Parameter>(k_NSkipLines_Key, "Skip Header Lines", "Number of lines at the start of the file to skip", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoice_Key, "Delimiter", "Delimiter for values on a line", 0,
                                                   ChoicesParameter::Choices{", (comma)", "; (semicolon)", "  (space)", ": (colon)", "\\t (Tab)"}));

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataArrayPath_Key, "Created Array Path", "DataPath or Name for the underlying Data Array", DataPath{}));
  params.insert(std::make_unique<DataStoreFormatParameter>(k_DataFormat_Key, "Data Format",
                                                           "This value will specify which data format is used by the array's data store. An empty string results in in-memory data store.", ""));

  params.insertSeparator(Parameters::Separator{"Tuple Handling"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_AdvancedOptions_Key, "Set Tuple Dimensions [not required if creating inside an Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix", true));

  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "DIM {}"));
  params.insert(std::make_unique<DynamicTableParameter>(k_NTuples_Key, "Data Array Dimensions (Slowest to Fastest Dimensions)",
                                                        "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", tableInfo));

  params.linkParameters(k_AdvancedOptions_Key, k_NTuples_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadTextDataArrayFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer ReadTextDataArrayFilter::clone() const
{
  return std::make_unique<ReadTextDataArrayFilter>();
}

IFilter::PreflightResult ReadTextDataArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto numericType = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto arrayPath = filterArgs.value<DataPath>(k_DataArrayPath_Key);
  auto nComp = filterArgs.value<uint64>(k_NComp_Key);

  auto useDims = filterArgs.value<bool>(k_AdvancedOptions_Key);
  auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_NTuples_Key);
  nx::core::Result<OutputActions> resultOutputActions;

  ShapeType tupleDims = {};

  auto* parentAM = dataStructure.getDataAs<AttributeMatrix>(arrayPath.getParent());
  if(parentAM == nullptr)
  {
    if(!useDims)
    {
      return MakePreflightErrorResult(
          -78602, fmt::format("The DataArray to be created '{}'is not within an AttributeMatrix, so the dimensions cannot be determined implicitly. Check Set Tuple Dimensions to set the dimensions",
                              arrayPath.toString()));
    }
    else
    {
      const std::vector<double>& rowData = tableData.at(0);
      tupleDims.reserve(rowData.size());
      for(size_t idx = 0; idx < rowData.size(); idx++)
      {
        if(rowData[idx] == 0)
        {
          return MakePreflightErrorResult(-78603, fmt::format("Tuple dimension at index {} cannot be 0", idx));
        }
        tupleDims.push_back(static_cast<usize>(rowData[idx]));
      }
    }
  }
  else
  {
    tupleDims = parentAM->getShape();
    if(useDims)
    {
      resultOutputActions.warnings().push_back(
          Warning{-77604, "You checked Set Tuple Dimensions, but selected a DataPath that has an Attribute Matrix as the parent. The Attribute Matrix tuples will override your "
                          "custom dimensions. It is recommended to uncheck Set Tuple Dimensions for the sake of clarity."});
    }
  }

  auto dataFormat = filterArgs.value<std::string>(k_DataFormat_Key);

  auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(numericType), tupleDims, std::vector<usize>{nComp}, arrayPath, dataFormat);

  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions)};
}

Result<> ReadTextDataArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadTextDataArrayInputValues inputValues;
  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.SkipLineCount = filterArgs.value<uint64>(k_NSkipLines_Key);
  inputValues.DelimiterIndex = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoice_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_DataArrayPath_Key);

  return ReadTextDataArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_ScalarTypeKey = "ScalarType";
constexpr StringLiteral k_NumberOfComponentsKey = "NumberOfComponents";
constexpr StringLiteral k_SkipHeaderLinesKey = "SkipHeaderLines";
constexpr StringLiteral k_DelimiterKey = "Delimiter";
constexpr StringLiteral k_CreatedAttributeArrayPathKey = "CreatedAttributeArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadTextDataArrayFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadTextDataArrayFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::NumericTypeParameterConverter>(args, json, SIMPL::k_ScalarTypeKey, k_ScalarType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_NumberOfComponentsKey, k_NComp_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_SkipHeaderLinesKey, k_NSkipLines_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DelimiterKey, k_DelimiterChoice_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationFilterParameterConverter>(args, json, SIMPL::k_CreatedAttributeArrayPathKey, k_DataArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
