#include "CalculateArrayHistogramFilter.hpp"
#include "Algorithms/CalculateArrayHistogram.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CalculateArrayHistogramFilter::name() const
{
  return FilterTraits<CalculateArrayHistogramFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CalculateArrayHistogramFilter::className() const
{
  return FilterTraits<CalculateArrayHistogramFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CalculateArrayHistogramFilter::uuid() const
{
  return FilterTraits<CalculateArrayHistogramFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CalculateArrayHistogramFilter::humanName() const
{
  return "Calculate Frequency Histogram";
}

//------------------------------------------------------------------------------
std::vector<std::string> CalculateArrayHistogramFilter::defaultTags() const
{
  return {"Statistics", "Ensemble"};
}

//------------------------------------------------------------------------------
Parameters CalculateArrayHistogramFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfBins_Key, "Number of Bins", "Specifies number of histogram bins (greater than zero)", 1));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UserDefinedRange_Key, "Use Custom Min & Max Range", "Whether the user can set the min and max values to consider for the histogram", false));
  params.insert(std::make_unique<Float64Parameter>(k_MinRange_Key, "Min Value", "Specifies the lower bound of the histogram.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_MaxRange_Key, "Max Value", "Specifies the upper bound of the histogram.", 1.0));
  params.insertSeparator(Parameters::Separator{"Input Arrays"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedArrayPaths_Key, "Input Data Arrays", "The list of arrays to calculate histogram(s) for",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               complex::GetAllNumericTypes()));
  params.insertSeparator(Parameters::Separator{"Output Set Up"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_NewDataGroup_Key, "Create New DataGroup for Histograms", "Whether or not to store the calculated histogram(s) in a new DataGroup", true));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataGroupName_Key, "New DataGroup Path", "The path to the new DataGroup in which to store the calculated histogram(s)", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataGroupName_Key, "Output DataGroup Path", "The complete path to the DataGroup in which to store the calculated histogram(s)",
                                                              DataPath{}, DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix, BaseGroup::GroupType::DataGroup}));
  params.insert(std::make_unique<StringParameter>(k_HistoName_Key, "Suffix for created Histograms", "String appended to the end of the histogram array names", " Histogram"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UserDefinedRange_Key, k_MinRange_Key, true);
  params.linkParameters(k_UserDefinedRange_Key, k_MaxRange_Key, true);
  params.linkParameters(k_NewDataGroup_Key, k_NewDataGroupName_Key, true);
  params.linkParameters(k_NewDataGroup_Key, k_DataGroupName_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CalculateArrayHistogramFilter::clone() const
{
  return std::make_unique<CalculateArrayHistogramFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CalculateArrayHistogramFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pUserDefinedRangeValue = filterArgs.value<bool>(k_UserDefinedRange_Key); // verify and calculate range values here if false
  auto pMinRangeValue = filterArgs.value<float64>(k_MinRange_Key);
  auto pMaxRangeValue = filterArgs.value<float64>(k_MaxRange_Key);
  auto pNewDataGroupValue = filterArgs.value<bool>(k_NewDataGroup_Key);
  auto pDataGroupNameValue = filterArgs.value<DataPath>(k_DataGroupName_Key);
  auto pSelectedArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedArrayPaths_Key);
  auto pNewDataGroupNameValue = filterArgs.value<DataPath>(k_NewDataGroupName_Key); // sanity check if is Attribute matrix after impending complex update
  auto pHistogramSuffix = filterArgs.value<std::string>(k_HistoName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(pNewDataGroupValue)
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pNewDataGroupNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  for(auto& selectedArrayPath : pSelectedArrayPathsValue)
  {
    const auto& dataArray = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
    DataPath childPath;
    if(pNewDataGroupValue)
    {
      childPath = pNewDataGroupNameValue.createChildPath((dataArray->getName() + pHistogramSuffix));
    }
    else
    {
      childPath = pDataGroupNameValue.createChildPath((dataArray->getName() + pHistogramSuffix));
    }
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{static_cast<usize>(pNumberOfBinsValue)}, std::vector<usize>{2}, childPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CalculateArrayHistogramFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  CalculateArrayHistogramInputValues inputValues;

  inputValues.NumberOfBins = filterArgs.value<int32>(k_NumberOfBins_Key);
  inputValues.UserDefinedRange = filterArgs.value<bool>(k_UserDefinedRange_Key);
  inputValues.MinRange = filterArgs.value<float64>(k_MinRange_Key);
  inputValues.MaxRange = filterArgs.value<float64>(k_MaxRange_Key);
  inputValues.SelectedArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedArrayPaths_Key);

  auto histogramSuffix = filterArgs.value<std::string>(k_HistoName_Key);

  DataPath dataGroupPath;
  if(filterArgs.value<bool>(k_NewDataGroup_Key))
  {
    dataGroupPath = filterArgs.value<DataPath>(k_NewDataGroupName_Key);
  }
  else
  {
    dataGroupPath = filterArgs.value<DataPath>(k_DataGroupName_Key);
  }
  std::vector<DataPath> createdDataPaths;
  for(auto& selectedArrayPath : inputValues.SelectedArrayPaths) // regenerate based on preflight
  {
    const auto& dataArray = dataStructure.getDataAs<IDataArray>(selectedArrayPath);
    auto childPath = dataGroupPath.createChildPath((dataArray->getName() + histogramSuffix));
    createdDataPaths.push_back(childPath);
  }

  inputValues.CreatedHistogramDataPaths = createdDataPaths;

  return CalculateArrayHistogram(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
