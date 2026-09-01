#include "ComputeArrayHistogramByFeatureFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeArrayHistogramByFeature.hpp"

#include <limits>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeArrayHistogramByFeatureFilter::name() const
{
  return FilterTraits<ComputeArrayHistogramByFeatureFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeArrayHistogramByFeatureFilter::className() const
{
  return FilterTraits<ComputeArrayHistogramByFeatureFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeArrayHistogramByFeatureFilter::uuid() const
{
  return FilterTraits<ComputeArrayHistogramByFeatureFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeArrayHistogramByFeatureFilter::humanName() const
{
  return "Compute Attribute Array Frequency Histogram (Feature)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeArrayHistogramByFeatureFilter::defaultTags() const
{
  return {className(), "Statistics", "Ensemble", "Histogram", "Feature"};
}

//------------------------------------------------------------------------------
Parameters ComputeArrayHistogramByFeatureFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfBins_Key, "Number of Bins", "Specifies number of histogram bins (greater than zero)", 10));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UserDefinedRange_Key, "Use Custom Min & Max Range", "Whether the user can set the min and max values to consider for the histogram", false));
  params.insert(std::make_unique<Float64Parameter>(k_MinRange_Key, "Min Value", "Specifies the inclusive lower bound of the histogram.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_MaxRange_Key, "Max Value", "Specifies the exclusive upper bound of the histogram.", 1.0));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CalculateModalBinRanges_Key, "Calculate Modal Bin Ranges", "Whether to compute the histogram bin ranges that contain the mode values.", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedArrayPaths_Key, "Input Data Arrays", "The list of arrays to calculate histogram(s) for",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               nx::core::GetAllNumericTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CreateNewDataGroup_Key, "Create New DataGroup for Histograms", "Whether or not to store the calculated histogram(s) in a new DataGroup", true));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataGroupPath_Key, "New DataGroup Path", "The path to the new DataGroup in which to store the calculated histogram(s)", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataGroupPath_Key, "Output DataGroup Path", "The complete path to the DataGroup in which to store the calculated histogram(s)",
                                                              DataPath{}, DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix, BaseGroup::GroupType::DataGroup}));
  params.insert(std::make_unique<StringParameter>(k_HistoBinCountName_Key, "Histogram Bin Counts Array Name", "Name of the created \"Bin Counts\" array for each input array.", "Bin Counts"));
  params.insert(std::make_unique<StringParameter>(k_HistoBinRangeName_Key, "Histogram Bin Ranges Array Name", "Name of the created \"Bin Ranges\" array for each input array.", "Bin Ranges"));
  params.insert(std::make_unique<StringParameter>(k_HistoMostPopulatedBinName_Key, "Most Populated Bin Array Name", "Name of the created \"Most Populated Bin\" array for each input array.",
                                                  "Most Populated Bin"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_HistoModalBinRangesName_Key, "Modal Bin Ranges Array Name", "Name of the created \"Modal Bin Ranges\" array for each input array.",
                                                          "Modal Bin Ranges"));

  params.linkParameters(k_UserDefinedRange_Key, k_MinRange_Key, true);
  params.linkParameters(k_UserDefinedRange_Key, k_MaxRange_Key, true);
  params.linkParameters(k_CreateNewDataGroup_Key, k_NewDataGroupPath_Key, true);
  params.linkParameters(k_CreateNewDataGroup_Key, k_DataGroupPath_Key, false);
  params.linkParameters(k_CalculateModalBinRanges_Key, k_HistoModalBinRangesName_Key, true);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeArrayHistogramByFeatureFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeArrayHistogramByFeatureFilter::clone() const
{
  return std::make_unique<ComputeArrayHistogramByFeatureFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeArrayHistogramByFeatureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pNewDataGroupValue = filterArgs.value<bool>(k_CreateNewDataGroup_Key);
  auto pDataGroupNameValue = filterArgs.value<DataPath>(k_DataGroupPath_Key);
  auto pSelectedArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedArrayPaths_Key);
  auto pNewDataGroupNameValue = filterArgs.value<DataPath>(k_NewDataGroupPath_Key); // sanity check if is Attribute matrix after impending simplnx update
  auto pBinCountName = filterArgs.value<std::string>(k_HistoBinCountName_Key);
  auto pBinRangeName = filterArgs.value<std::string>(k_HistoBinRangeName_Key);
  auto pBinMostPopulatedName = filterArgs.value<std::string>(k_HistoMostPopulatedBinName_Key);
  auto pCalculateModalBinRanges = filterArgs.value<bool>(k_CalculateModalBinRanges_Key);
  auto pBinModalBinRangesName = filterArgs.value<std::string>(k_HistoModalBinRangesName_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCellFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  if(pNumberOfBinsValue <= 0)
  {
    return {MakeErrorResult<OutputActions>(-57208, fmt::format("Number of Bins ({}) must be greater than zero.", pNumberOfBinsValue)), {}};
  }
  if(static_cast<usize>(pNumberOfBinsValue) > std::numeric_limits<usize>::max() / 2)
  {
    return {MakeErrorResult<OutputActions>(-57210, fmt::format("Number of Bins ({}) is too large to create a bin-range component shape with two values per bin on this platform.", pNumberOfBinsValue)),
            {}};
  }

  const auto& featureIdsArray = dataStructure.getDataRefAs<IDataArray>(pCellFeatureIdsArrayPathValue);

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

  const IDataArray* maskArray = nullptr;
  if(pUseMaskValue)
  {
    maskArray = dataStructure.getDataAs<IDataArray>(pMaskArrayPathValue);
  }

  for(auto& selectedArrayPath : pSelectedArrayPathsValue)
  {
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
    if(featureIdsArray.getNumberOfTuples() != dataArray->getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-57209,
                                             fmt::format("Cell Feature Ids array '{}' has tuple count {} and input array '{}' has tuple count {}. These tuple counts MUST match.",
                                                         pCellFeatureIdsArrayPathValue.toString(), featureIdsArray.getNumberOfTuples(), selectedArrayPath.toString(), dataArray->getNumberOfTuples())),
              {}};
    }
    if(maskArray && maskArray->getNumberOfTuples() != dataArray->getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-57207, fmt::format("Mask array '{}' has tuple count {} and input array '{}' has tuple count {}. These tuple counts MUST match.",
                                                                 pMaskArrayPathValue.toString(), maskArray->getNumberOfTuples(), selectedArrayPath.toString(), dataArray->getNumberOfTuples())),
              {}};
    }

    auto arrayGroupPath = parentPath.createChildPath(fmt::format("\"{}\" Histogram", dataArray->getName()));
    resultOutputActions.value().appendAction(std::make_unique<CreateDataGroupAction>(arrayGroupPath));

    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::uint64, std::vector<usize>{1}, std::vector<usize>{static_cast<usize>(pNumberOfBinsValue)},
                                                                   arrayGroupPath.createChildPath(pBinCountName));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }

    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(dataArray->getDataType(), std::vector<usize>{1}, std::vector<usize>{static_cast<usize>(pNumberOfBinsValue) * 2},
                                                                   arrayGroupPath.createChildPath(pBinRangeName));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }

    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::uint64, std::vector<usize>{1}, std::vector<usize>{2}, arrayGroupPath.createChildPath(pBinMostPopulatedName));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    if(pCalculateModalBinRanges)
    {
      auto createArrayAction = std::make_unique<CreateNeighborListAction>(dataArray->getDataType(), std::vector<usize>{1}, arrayGroupPath.createChildPath(pBinModalBinRangesName));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeArrayHistogramByFeatureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeArrayHistogramByFeatureInputValues inputValues;

  inputValues.NumberOfBins = filterArgs.value<int32>(k_NumberOfBins_Key);
  inputValues.UserDefinedRange = filterArgs.value<bool>(k_UserDefinedRange_Key);
  inputValues.MinRange = filterArgs.value<float64>(k_MinRange_Key);
  inputValues.MaxRange = filterArgs.value<float64>(k_MaxRange_Key);
  inputValues.SelectedArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedArrayPaths_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);

  auto binCountName = filterArgs.value<std::string>(k_HistoBinCountName_Key);
  auto binRangeName = filterArgs.value<std::string>(k_HistoBinRangeName_Key);
  auto binMostPopulatedName = filterArgs.value<std::string>(k_HistoMostPopulatedBinName_Key);
  auto calculateModalRanges = filterArgs.value<bool>(k_CalculateModalBinRanges_Key);
  auto binModalRangesName = filterArgs.value<std::string>(k_HistoModalBinRangesName_Key);

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
  std::vector<DataPath> createdMostPopulatedDataPaths;
  std::vector<DataPath> createdModalRangesDataPaths;
  for(auto& selectedArrayPath : inputValues.SelectedArrayPaths) // regenerate based on preflight
  {
    const auto& dataArray = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
    auto arrayGroupPath = dataGroupPath.createChildPath(fmt::format("\"{}\" Histogram", dataArray->getName()));
    auto countsPath = arrayGroupPath.createChildPath(binCountName);
    createdCountsDataPaths.push_back(countsPath);
    auto rangesPath = arrayGroupPath.createChildPath(binRangeName);
    createdRangesDataPaths.push_back(rangesPath);
    auto mostPopulatedPath = arrayGroupPath.createChildPath(binMostPopulatedName);
    createdMostPopulatedDataPaths.push_back(mostPopulatedPath);

    if(calculateModalRanges)
    {
      auto modalRangesPath = arrayGroupPath.createChildPath(binModalRangesName);
      createdModalRangesDataPaths.push_back(modalRangesPath);
    }
  }

  inputValues.CreatedHistogramCountsDataPaths = createdCountsDataPaths;
  inputValues.CreatedBinRangeDataPaths = createdRangesDataPaths;
  inputValues.CreatedBinMostPopulatedDataPaths = createdMostPopulatedDataPaths;
  if(calculateModalRanges)
  {
    inputValues.CreatedBinModalRangesDataPaths = createdModalRangesDataPaths;
  }

  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return ComputeArrayHistogramByFeature(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
