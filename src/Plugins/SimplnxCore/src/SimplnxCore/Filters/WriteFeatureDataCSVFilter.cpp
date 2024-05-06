#include "WriteFeatureDataCSVFilter.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"

#include <filesystem>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fstream>

namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteFeatureDataCSVFilter::name() const
{
  return FilterTraits<WriteFeatureDataCSVFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteFeatureDataCSVFilter::className() const
{
  return FilterTraits<WriteFeatureDataCSVFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteFeatureDataCSVFilter::uuid() const
{
  return FilterTraits<WriteFeatureDataCSVFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteFeatureDataCSVFilter::humanName() const
{
  return "Write Feature Data as CSV File";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteFeatureDataCSVFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters WriteFeatureDataCSVFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_FeatureDataFile_Key, "Output File", "Path to the output file to write.", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteNeighborListData_Key, "Write Neighbor Data", "Should the neighbor list data be written to the file", true));
  params.insert(std::make_unique<BoolParameter>(k_WriteNumFeaturesLine_Key, "Write Number of Features Line", "Should the number of features be written to the file.", true));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceInt_Key, "Delimiter", "Default Delimiter is Comma", to_underlying(OStreamUtilities::Delimiter::Comma),
                                                   ChoicesParameter::Choices{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "Input Feature Attribute Matrix", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteFeatureDataCSVFilter::clone() const
{
  return std::make_unique<WriteFeatureDataCSVFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteFeatureDataCSVFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> WriteFeatureDataCSVFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  auto atomicFileResult = AtomicFile::Create(filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key));
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  auto pOutputFilePath = atomicFile.tempFilePath();
  auto pWriteNeighborListDataValue = filterArgs.value<bool>(k_WriteNeighborListData_Key);
  auto pWriteNumFeaturesLineValue = filterArgs.value<bool>(k_WriteNumFeaturesLine_Key);
  auto pDelimiterChoiceIntValue = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  const std::string delimiter = OStreamUtilities::DelimiterToString(pDelimiterChoiceIntValue);

  auto& cellFeatureAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue);

  // Ensure the complete path to the output file exists or can be created
  auto parentPath = pOutputFilePath.parent_path();
  if(!std::filesystem::exists(parentPath))
  {
    if(!std::filesystem::create_directories(parentPath))
    {
      return MakeErrorResult(-64641, fmt::format("Error creating Output file at path '{}'. Parent path could not be created.", pOutputFilePath.string()));
    }
  }

  // load list of DataPaths
  std::vector<DataObject::Type> dataTypesToExtract;
  if(filterArgs.value<bool>(k_WriteNeighborListData_Key))
  {
    dataTypesToExtract = {DataObject::Type::DataArray, DataObject::Type::StringArray, DataObject::Type::NeighborList};
  }
  else
  {
    dataTypesToExtract = {DataObject::Type::DataArray, DataObject::Type::StringArray};
  }

  std::vector<DataPath> arrayPaths;
  std::vector<DataPath> neighborPaths;
  for(const auto& element : dataTypesToExtract)
  {
    auto requestedPaths = *std::move(GetAllChildDataPaths(dataStructure, pCellFeatureAttributeMatrixPathValue, element));
    if(element == DataObject::Type::NeighborList)
    {
      neighborPaths.insert(neighborPaths.end(), std::make_move_iterator(requestedPaths.begin()), std::make_move_iterator(requestedPaths.end()));
    }
    else
    {
      arrayPaths.insert(arrayPaths.end(), std::make_move_iterator(requestedPaths.begin()), std::make_move_iterator(requestedPaths.end()));
    }
  }

  // Scope file writer in code block to get around file lock on windows (enforce destructor order)
  {
    std::ofstream fout(pOutputFilePath.string(), std::ofstream::out | std::ios_base::binary); // test name resolution and create file
    if(!fout.is_open())
    {
      return MakeErrorResult(-64640, fmt::format("Error opening path {}", pOutputFilePath.string()));
    }

    // call ostream function
    OStreamUtilities::PrintDataSetsToSingleFile(fout, arrayPaths, dataStructure, messageHandler, shouldCancel, delimiter, true, true, false, "Feature_ID", neighborPaths, pWriteNumFeaturesLineValue);
  }

  Result<> commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    return commitResult;
  }
  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureDataFileKey = "FeatureDataFile";
constexpr StringLiteral k_WriteNeighborListDataKey = "WriteNeighborListData";
constexpr StringLiteral k_WriteNumFeaturesLineKey = "WriteNumFeaturesLine";
constexpr StringLiteral k_DelimiterChoiceIntKey = "DelimiterChoiceInt";
constexpr StringLiteral k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
} // namespace SIMPL
} // namespace

Result<Arguments> WriteFeatureDataCSVFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteFeatureDataCSVFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_FeatureDataFileKey, k_FeatureDataFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_WriteNeighborListDataKey, k_WriteNeighborListData_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_WriteNumFeaturesLineKey, k_WriteNumFeaturesLine_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DelimiterChoiceIntKey, k_DelimiterChoiceInt_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellFeatureAttributeMatrixPathKey,
                                                                                                                         k_CellFeatureAttributeMatrixPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
