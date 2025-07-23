#include "ReadStringDataArrayFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadStringDataArray.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataStoreFormatParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
} // namespace

namespace nx::core
{
std::string ReadStringDataArrayFilter::name() const
{
  return FilterTraits<ReadStringDataArrayFilter>::name;
}

std::string ReadStringDataArrayFilter::className() const
{
  return FilterTraits<ReadStringDataArrayFilter>::className;
}

Uuid ReadStringDataArrayFilter::uuid() const
{
  return FilterTraits<ReadStringDataArrayFilter>::uuid;
}

std::vector<std::string> ReadStringDataArrayFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Text", "ASCII", "Attribute", "String"};
}

std::string ReadStringDataArrayFilter::humanName() const
{
  return "Read String Data Array";
}

Parameters ReadStringDataArrayFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "File path that points to the imported file", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<UInt64Parameter>(k_NSkipLines_Key, "Skip Header Lines", "Number of lines at the start of the file to skip", 0));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoice_Key, "Delimiter", "Delimiter for values on a line", 0, nx::core::read_string_data_array::k_Delimiters));

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DataArrayPath_Key, "Created Array Path", "DataPath or Name for the underlying Data Array", DataPath{}));
  params.insert(std::make_unique<DataStoreFormatParameter>(k_DataFormat_Key, "Data Format",
                                                           "This value will specify which data format is used by the array's data store. An empty string results in in-memory data store.", ""));

  params.insertSeparator(Parameters::Separator{"Tuple Handling"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_SetTupleDimensions, "Set Tuple Dimensions [not required if creating inside an Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix", true));

  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "DIM {}"));
  params.insert(std::make_unique<DynamicTableParameter>(k_NTuples_Key, "Data Array Dimensions (Slowest to Fastest Dimensions)",
                                                        "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", tableInfo));

  params.linkParameters(k_SetTupleDimensions, k_NTuples_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadStringDataArrayFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer ReadStringDataArrayFilter::clone() const
{
  return std::make_unique<ReadStringDataArrayFilter>();
}

IFilter::PreflightResult ReadStringDataArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto arrayPath = filterArgs.value<DataPath>(k_DataArrayPath_Key);

  auto useDims = filterArgs.value<bool>(k_SetTupleDimensions);
  auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_NTuples_Key);
  auto dataFormat = filterArgs.value<std::string>(k_DataFormat_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<usize> tupleDims = {};

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
      const auto& rowData = tableData.at(0);
      tupleDims.reserve(rowData.size());
      for(auto floatValue : rowData)
      {
        if(floatValue == 0)
        {
          return MakePreflightErrorResult(-77603, "Tuple dimension cannot be zero");
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
          Warning{-77604, "You checked Set Tuple Dimensions, but selected a DataPath that has an Attribute Matrix as the parent. The Attribute Matrix tuples will override your "
                          "custom dimensions. It is recommended to uncheck Set Tuple Dimensions for the sake of clarity."});
    }
  }

  auto action = std::make_unique<CreateStringArrayAction>(tupleDims, arrayPath);

  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions)};
}

Result<> ReadStringDataArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadStringDataArrayInputValues inputValues;

  inputValues.inputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.skipLineCount = filterArgs.value<UInt64Parameter::ValueType>(k_NSkipLines_Key);
  inputValues.delimiterIndex = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoice_Key);
  inputValues.outputArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_DataArrayPath_Key);
  inputValues.dataFormat = filterArgs.value<DataStoreFormatParameter::ValueType>(k_DataFormat_Key);

  return ReadStringDataArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

} // namespace nx::core
