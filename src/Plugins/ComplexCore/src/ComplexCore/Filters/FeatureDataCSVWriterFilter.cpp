#include "FeatureDataCSVWriterFilter.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/OStreamUtilities.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FeatureDataCSVWriterFilter::name() const
{
  return FilterTraits<FeatureDataCSVWriterFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FeatureDataCSVWriterFilter::className() const
{
  return FilterTraits<FeatureDataCSVWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FeatureDataCSVWriterFilter::uuid() const
{
  return FilterTraits<FeatureDataCSVWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FeatureDataCSVWriterFilter::humanName() const
{
  return "Export Feature Data as CSV File";
}

//------------------------------------------------------------------------------
std::vector<std::string> FeatureDataCSVWriterFilter::defaultTags() const
{
  return {"#IO", "#Output", "#Write", "#Export"};
}

//------------------------------------------------------------------------------
Parameters FeatureDataCSVWriterFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_FeatureDataFile_Key, "Output File", "", fs::path(""), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<BoolParameter>(k_WriteNeighborListData_Key, "Write Neighbor Data", "", true));
  params.insert(std::make_unique<BoolParameter>(k_WriteNumFeaturesLine_Key, "Write Number of Features Line", "", true));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceInt_Key, "Delimiter", "Default Delimiter is Comma", to_underlying(OStreamUtilities::Delimiter::Comma),
                                                   ChoicesParameter::Choices{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FeatureDataCSVWriterFilter::clone() const
{
  return std::make_unique<FeatureDataCSVWriterFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FeatureDataCSVWriterFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pFeatureDataFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
  auto pWriteNeighborListDataValue = filterArgs.value<bool>(k_WriteNeighborListData_Key);
  auto pWriteNumFeaturesLineValue = filterArgs.value<bool>(k_WriteNumFeaturesLine_Key);
  auto pDelimiterChoiceIntValue = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FeatureDataCSVWriterFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto pOutputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
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

  // load list of datapaths
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

  std::ofstream fout(pOutputFilePath.string(), std::ofstream::out); // test name resolution and create file
  if(!fout.is_open())
  {
    return MakeErrorResult(-64640, fmt::format("Error opening path {}", pOutputFilePath.string()));
  }

  // call ostream function
  OStreamUtilities::PrintDataSetsToSingleFile(fout, arrayPaths, dataStructure, messageHandler, shouldCancel, delimiter, true, true, neighborPaths, pWriteNumFeaturesLineValue);

  return {};
}
} // namespace complex
