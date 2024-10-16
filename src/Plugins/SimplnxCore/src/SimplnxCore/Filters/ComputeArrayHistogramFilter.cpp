#include "ComputeArrayHistogramFilter.hpp"
#include "Algorithms/ComputeArrayHistogram.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/StringParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeArrayHistogramFilter::name() const
{
  return FilterTraits<ComputeArrayHistogramFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeArrayHistogramFilter::className() const
{
  return FilterTraits<ComputeArrayHistogramFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeArrayHistogramFilter::uuid() const
{
  return FilterTraits<ComputeArrayHistogramFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeArrayHistogramFilter::humanName() const
{
  return "Compute Attribute Array Frequency Histogram";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeArrayHistogramFilter::defaultTags() const
{
  return {className(), "Statistics", "Ensemble", "Histogram"};
}

//------------------------------------------------------------------------------
Parameters ComputeArrayHistogramFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfBins_Key, "Number of Bins", "Specifies number of histogram bins (greater than zero)", 10));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UserDefinedRange_Key, "Use Custom Min & Max Range", "Whether the user can set the min and max values to consider for the histogram", false));
  params.insert(std::make_unique<Float64Parameter>(k_MinRange_Key, "Min Value", "Specifies the inclusive lower bound of the histogram.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_MaxRange_Key, "Max Value", "Specifies the exclusive upper bound of the histogram.", 1.0));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedArrayPaths_Key, "Input Data Arrays", "The list of arrays to calculate histogram(s) for",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               nx::core::GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CreateNewDataGroup_Key, "Create New DataGroup for Histograms", "Whether or not to store the calculated histogram(s) in a new DataGroup", true));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataGroupPath_Key, "New DataGroup Path", "The path to the new DataGroup in which to store the calculated histogram(s)", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataGroupPath_Key, "Output DataGroup Path", "The complete path to the DataGroup in which to store the calculated histogram(s)",
                                                              DataPath{}, DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix, BaseGroup::GroupType::DataGroup}));
  params.insert(std::make_unique<StringParameter>(k_HistoBinCountName_Key, "Suffix for created Histogram Bin Counts", "String appended to the end of the histogram array names", " Histogram Counts"));
  params.insert(
      std::make_unique<StringParameter>(k_HistoBinRangeName_Key, "Suffix for created Histogram Bin Ranges", "String appended to the end of the histogram array names", " Histogram Bin Ranges"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UserDefinedRange_Key, k_MinRange_Key, true);
  params.linkParameters(k_UserDefinedRange_Key, k_MaxRange_Key, true);
  params.linkParameters(k_CreateNewDataGroup_Key, k_NewDataGroupPath_Key, true);
  params.linkParameters(k_CreateNewDataGroup_Key, k_DataGroupPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeArrayHistogramFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeArrayHistogramFilter::clone() const
{
  return std::make_unique<ComputeArrayHistogramFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeArrayHistogramFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pNewDataGroupValue = filterArgs.value<bool>(k_CreateNewDataGroup_Key);
  auto pDataGroupNameValue = filterArgs.value<DataPath>(k_DataGroupPath_Key);
  auto pSelectedArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedArrayPaths_Key);
  auto pNewDataGroupNameValue = filterArgs.value<DataPath>(k_NewDataGroupPath_Key); // sanity check if is Attribute matrix after impending simplnx update
  auto pBinCountSuffix = filterArgs.value<std::string>(k_HistoBinCountName_Key);
  auto pBinRangeSuffix = filterArgs.value<std::string>(k_HistoBinRangeName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  if(pNewDataGroupValue)
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pNewDataGroupNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  DataPath parentPath = {};
  if(pNewDataGroupValue)
  {
    parentPath = pNewDataGroupNameValue;
  }
  else
  {
    parentPath = pDataGroupNameValue;
  }
  for(auto& selectedArrayPath : pSelectedArrayPathsValue)
  {
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::uint64, std::vector<usize>{static_cast<usize>(pNumberOfBinsValue)}, std::vector<usize>{1},
                                                                   parentPath.createChildPath((dataArray->getName() + pBinCountSuffix)));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }

    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(dataArray->getDataType(), std::vector<usize>{static_cast<usize>(pNumberOfBinsValue)}, std::vector<usize>{2},
                                                                   parentPath.createChildPath((dataArray->getName() + pBinRangeSuffix)));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeArrayHistogramFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ComputeArrayHistogramInputValues inputValues;

  inputValues.NumberOfBins = filterArgs.value<int32>(k_NumberOfBins_Key);
  inputValues.UserDefinedRange = filterArgs.value<bool>(k_UserDefinedRange_Key);
  inputValues.MinRange = filterArgs.value<float64>(k_MinRange_Key);
  inputValues.MaxRange = filterArgs.value<float64>(k_MaxRange_Key);
  inputValues.SelectedArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedArrayPaths_Key);

  auto binCountSuffix = filterArgs.value<std::string>(k_HistoBinCountName_Key);
  auto binRangeSuffix = filterArgs.value<std::string>(k_HistoBinRangeName_Key);

  DataPath dataGroupPath;
  if(filterArgs.value<bool>(k_CreateNewDataGroup_Key))
  {
    dataGroupPath = filterArgs.value<DataPath>(k_NewDataGroupPath_Key);
  }
  else
  {
    dataGroupPath = filterArgs.value<DataPath>(k_DataGroupPath_Key);
  }
  std::vector<DataPath> createdCountsDataPaths;
  std::vector<DataPath> createdRangesDataPaths;
  for(auto& selectedArrayPath : inputValues.SelectedArrayPaths) // regenerate based on preflight
  {
    const auto& dataArray = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
    auto countsPath = dataGroupPath.createChildPath((dataArray->getName() + binCountSuffix));
    createdCountsDataPaths.push_back(countsPath);
    auto rangesPath = dataGroupPath.createChildPath((dataArray->getName() + binRangeSuffix));
    createdRangesDataPaths.push_back(rangesPath);
  }

  inputValues.CreatedHistogramCountsDataPaths = createdCountsDataPaths;
  inputValues.CreatedBinRangeDataPaths = createdRangesDataPaths;

  return ComputeArrayHistogram(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_NumberOfBinsKey = "NumberOfBins";
constexpr StringLiteral k_UserDefinedRangeKey = "UserDefinedRange";
constexpr StringLiteral k_MinRangeKey = "MinRange";
constexpr StringLiteral k_MaxRangeKey = "MaxRange";
constexpr StringLiteral k_NewDataContainerKey = "NewDataContainer";
constexpr StringLiteral k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteral k_NewDataContainerNameKey = "NewDataContainerName";
constexpr StringLiteral k_NewAttributeMatrixNameKey = "NewAttributeMatrixName";
constexpr StringLiteral k_NewDataArrayNameKey = "NewDataArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeArrayHistogramFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeArrayHistogramFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumberOfBinsKey, k_NumberOfBins_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UserDefinedRangeKey, k_UserDefinedRange_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_MinRangeKey, k_MinRange_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_MaxRangeKey, k_MaxRange_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_NewDataContainerKey, k_CreateNewDataGroup_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_SelectedArrayPaths_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_NewDataContainerNameKey, k_NewDataGroupPath_Key));
  results.push_back(SIMPLConversion::Convert2Parameters<SIMPLConversion::AMPathBuilderFilterParameterConverter>(args, json, SIMPL::k_NewDataContainerNameKey, SIMPL::k_NewAttributeMatrixNameKey,
                                                                                                                k_DataGroupPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NewDataArrayNameKey, k_HistoBinCountName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
