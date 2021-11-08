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
  auto pImportHDF5FileValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ImportHDF5DatasetAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
