#include "DataContainerReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataContainerReaderFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string DataContainerReader::name() const
{
  return FilterTraits<DataContainerReader>::name.str();
}

//------------------------------------------------------------------------------
std::string DataContainerReader::className() const
{
  return FilterTraits<DataContainerReader>::className;
}

//------------------------------------------------------------------------------
Uuid DataContainerReader::uuid() const
{
  return FilterTraits<DataContainerReader>::uuid;
}

//------------------------------------------------------------------------------
std::string DataContainerReader::humanName() const
{
  return "Read DREAM.3D Data File";
}

//------------------------------------------------------------------------------
std::vector<std::string> DataContainerReader::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters DataContainerReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_OverwriteExistingDataContainers_Key, "Overwrite Existing Data Containers", "", false));
  /*[x]*/ params.insert(std::make_unique<DataContainerReaderFilterParameter>(k_InputFileDataContainerArrayProxy_Key, "Select Arrays from Input File", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DataContainerReader::clone() const
{
  return std::make_unique<DataContainerReader>();
}

//------------------------------------------------------------------------------
Result<OutputActions> DataContainerReader::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOverwriteExistingDataContainersValue = filterArgs.value<bool>(k_OverwriteExistingDataContainers_Key);
  auto pInputFileDataContainerArrayProxyValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileDataContainerArrayProxy_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<DataContainerReaderAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> DataContainerReader::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOverwriteExistingDataContainersValue = filterArgs.value<bool>(k_OverwriteExistingDataContainers_Key);
  auto pInputFileDataContainerArrayProxyValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileDataContainerArrayProxy_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
