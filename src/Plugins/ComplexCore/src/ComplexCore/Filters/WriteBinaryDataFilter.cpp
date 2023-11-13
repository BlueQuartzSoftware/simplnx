#include "WriteBinaryDataFilter.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/OStreamUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

namespace
{
struct ByteSwapArray
{
  template <typename ScalarType>
  Result<> operator()(IDataArray* inputDataArray)
  {
    if constexpr(std::is_same_v<ScalarType, bool> || std::is_same_v<ScalarType, uint8> || std::is_same_v<ScalarType, int8>) // byte-swap unnecessary bail early
    {
      return {};
    }
    auto* dataArray = dynamic_cast<DataArray<ScalarType>*>(inputDataArray);
    dataArray->byteSwapElements();
    return {};
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::name() const
{
  return FilterTraits<WriteBinaryDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::className() const
{
  return FilterTraits<WriteBinaryDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteBinaryDataFilter::uuid() const
{
  return FilterTraits<WriteBinaryDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteBinaryDataFilter::humanName() const
{
  return "Write Binary Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteBinaryDataFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Binary"};
}

//------------------------------------------------------------------------------
Parameters WriteBinaryDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<ChoicesParameter>(k_Endianess_Key, "Endianess", "Default is little endian", to_underlying(Endianess::Little),
                                                   ChoicesParameter::Choices{"Little Endian", "Big Endian"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Path", "The output file path", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_FileExtension_Key, "File Extension", "The file extension for the output file", ".bin"));
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Export", "The arrays to be exported to a binary file",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               complex::GetAllDataTypes()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteBinaryDataFilter::clone() const
{
  return std::make_unique<WriteBinaryDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteBinaryDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pEndianessValue = filterArgs.value<ChoicesParameter::ValueType>(k_Endianess_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pFileExtensionValue = filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteBinaryDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  const auto endianess = static_cast<endian>(filterArgs.value<ChoicesParameter::ValueType>(k_Endianess_Key));
  auto selectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  for(const auto& selectedArrayPath : selectedDataArrayPaths)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(endian::native != endianess) // if requested endianess is not native then byteswap
    {
      auto* oldSelectedArray = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
      ExecuteDataFunction(ByteSwapArray{}, oldSelectedArray->getDataType(), oldSelectedArray);
    }
  }

  fs::path dirPath(filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key));
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = complex::CreateOutputDirectories(dirPath);
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  if(!fs::is_directory(dirPath))
  {
    return MakeErrorResult(-23430, fmt::format("{}({}): Function {}: Error. OutputPath must be a directory. '{}'", "WriteBinaryDataFilter::executeImpl", __FILE__, __LINE__, dirPath.string()));
  }
  OStreamUtilities::PrintDataSetsToMultipleFiles(selectedDataArrayPaths, dataStructure, dirPath.string(), messageHandler, shouldCancel,
                                                 filterArgs.value<StringParameter::ValueType>(k_FileExtension_Key), true);
  return {};
}
} // namespace complex
