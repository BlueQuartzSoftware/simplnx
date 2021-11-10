#include "ImportHDF5Dataset.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/ImportHDF5DatasetFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportHDF5Dataset::name() const
{
  return FilterTraits<ImportHDF5Dataset>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportHDF5Dataset::className() const
{
  return FilterTraits<ImportHDF5Dataset>::className;
}

//------------------------------------------------------------------------------
Uuid ImportHDF5Dataset::uuid() const
{
  return FilterTraits<ImportHDF5Dataset>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportHDF5Dataset::humanName() const
{
  return "Import HDF5 Dataset";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportHDF5Dataset::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportHDF5Dataset::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<ImportHDF5DatasetFilterParameter>(k_ImportHDF5File_Key, "Select HDF5 File", "", {}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrix_Key, "Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportHDF5Dataset::clone() const
{
  return std::make_unique<ImportHDF5Dataset>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportHDF5Dataset::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pImportHDF5FileValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);

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
  auto action = std::make_unique<ImportHDF5DatasetAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ImportHDF5Dataset::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pImportHDF5FileValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
