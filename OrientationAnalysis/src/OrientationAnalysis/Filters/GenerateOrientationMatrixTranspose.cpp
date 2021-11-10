#include "GenerateOrientationMatrixTranspose.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateOrientationMatrixTranspose::name() const
{
  return FilterTraits<GenerateOrientationMatrixTranspose>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateOrientationMatrixTranspose::className() const
{
  return FilterTraits<GenerateOrientationMatrixTranspose>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateOrientationMatrixTranspose::uuid() const
{
  return FilterTraits<GenerateOrientationMatrixTranspose>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateOrientationMatrixTranspose::humanName() const
{
  return "Generate Orientation Matrix Transpose";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateOrientationMatrixTranspose::defaultTags() const
{
  return {"#Processing", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateOrientationMatrixTranspose::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_OrientationMatrixDataArrayPath_Key, "Quaternion Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Data Array Path", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateOrientationMatrixTranspose::clone() const
{
  return std::make_unique<GenerateOrientationMatrixTranspose>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateOrientationMatrixTranspose::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOrientationMatrixDataArrayPathValue = filterArgs.value<DataPath>(k_OrientationMatrixDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<GenerateOrientationMatrixTransposeAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> GenerateOrientationMatrixTranspose::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOrientationMatrixDataArrayPathValue = filterArgs.value<DataPath>(k_OrientationMatrixDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
