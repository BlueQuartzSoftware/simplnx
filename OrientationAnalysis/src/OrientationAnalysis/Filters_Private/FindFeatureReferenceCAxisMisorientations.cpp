#include "FindFeatureReferenceCAxisMisorientations.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientations::name() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientations>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientations::className() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientations>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureReferenceCAxisMisorientations::uuid() const
{
  return FilterTraits<FindFeatureReferenceCAxisMisorientations>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceCAxisMisorientations::humanName() const
{
  return "Find Feature Reference C-Axis Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureReferenceCAxisMisorientations::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureReferenceCAxisMisorientations::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureAvgCAxisMisorientationsArrayName_Key, "Average C-Axis Misorientations", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureStdevCAxisMisorientationsArrayName_Key, "Feature Stdev C-Axis Misorientations", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureReferenceCAxisMisorientationsArrayName_Key, "Feature Reference C-Axis Misorientations", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureReferenceCAxisMisorientations::clone() const
{
  return std::make_unique<FindFeatureReferenceCAxisMisorientations>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureReferenceCAxisMisorientations::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pFeatureAvgCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  auto pFeatureStdevCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  auto pFeatureReferenceCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureReferenceCAxisMisorientations::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pFeatureAvgCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  auto pFeatureStdevCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  auto pFeatureReferenceCAxisMisorientationsArrayNameValue = filterArgs.value<DataPath>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
