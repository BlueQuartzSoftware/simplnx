#include "WriteFeatureDataCSVFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteFeatureDataCSV.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

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
  params.insert(std::make_unique<FileSystemPathParameter>(k_FeatureDataFile_Key, "Output File", "Path to the output file to write.", fs::path("feature_data.csv"),
                                                          FileSystemPathParameter::ExtensionsType{".csv"}, FileSystemPathParameter::PathType::OutputFile, true));
  params.insert(std::make_unique<BoolParameter>(k_WriteNeighborListData_Key, "Write Neighbor Data", "Should the neighbor list data be written to the file", true));
  params.insert(std::make_unique<BoolParameter>(k_WriteNumFeaturesLine_Key, "Write Number of Features Line", "Should the number of features be written to the file.", true));
  params.insert(std::make_unique<ChoicesParameter>(k_DelimiterChoiceInt_Key, "Delimiter", "Default Delimiter is Comma", to_underlying(o_stream_utilities::Delimiter::Comma),
                                                   ChoicesParameter::ChoicesType{"Space", "Semicolon", "Comma", "Colon", "Tab"})); // sequence dependent DO NOT REORDER
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "Input Feature Attribute Matrix", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteFeatureDataCSVFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteFeatureDataCSVFilter::clone() const
{
  return std::make_unique<WriteFeatureDataCSVFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteFeatureDataCSVFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& /*filterArgs*/, const MessageHandler& /*messageHandler*/,
                                                                  const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  return {};
}

//------------------------------------------------------------------------------
Result<> WriteFeatureDataCSVFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  WriteFeatureDataCSVInputValues inputValues;
  inputValues.feature_data_file = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
  inputValues.write_num_features_line = filterArgs.value<BoolParameter::ValueType>(k_WriteNumFeaturesLine_Key);
  inputValues.delimiter_index = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  inputValues.cell_feature_attribute_matrix_path = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.write_neighborlist_data = filterArgs.value<BoolParameter::ValueType>(k_WriteNeighborListData_Key);

  return WriteFeatureDataCSV(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_FeatureDataFileKey = "FeatureDataFile";
constexpr StringLiteralType k_WriteNeighborListDataKey = "WriteNeighborListData";
constexpr StringLiteralType k_WriteNumFeaturesLineKey = "WriteNumFeaturesLine";
constexpr StringLiteralType k_DelimiterChoiceIntKey = "DelimiterChoiceInt";
constexpr StringLiteralType k_CellFeatureAttributeMatrixPathKey = "CellFeatureAttributeMatrixPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteFeatureDataCSVFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteFeatureDataCSVFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_FeatureDataFileKey, k_FeatureDataFile_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_WriteNeighborListDataKey, k_WriteNeighborListData_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_WriteNumFeaturesLineKey, k_WriteNumFeaturesLine_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_DelimiterChoiceIntKey, k_DelimiterChoiceInt_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_CellFeatureAttributeMatrixPathKey,
                                                                                                                         k_CellFeatureAttributeMatrixPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
