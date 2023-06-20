#include "RenameDataObject.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/RenameDataAction.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <stdexcept>

using namespace complex;

namespace
{
constexpr int32 k_EmptyParameterError = -520;
} // namespace

namespace complex
{

//------------------------------------------------------------------------------
std::string RenameDataObject::name() const
{
  return FilterTraits<RenameDataObject>::name;
}

//------------------------------------------------------------------------------
std::string RenameDataObject::className() const
{
  return FilterTraits<RenameDataObject>::className;
}

//------------------------------------------------------------------------------
Uuid RenameDataObject::uuid() const
{
  return FilterTraits<RenameDataObject>::uuid;
}

//------------------------------------------------------------------------------
std::string RenameDataObject::humanName() const
{
  return "Rename DataObject";
}

//------------------------------------------------------------------------------
std::vector<std::string> RenameDataObject::defaultTags() const
{
  return {"Data Management", "Rename", "Data Structure", "Data Object"};
}

//------------------------------------------------------------------------------
Parameters RenameDataObject::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_DataObject_Key, "DataObject to Rename", "DataPath pointing to the target DataObject", DataPath()));
  params.insert(std::make_unique<StringParameter>(k_NewName_Key, "New Name", "Target DataObject name", ""));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RenameDataObject::clone() const
{
  return std::make_unique<RenameDataObject>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RenameDataObject::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto dataGroupPath = filterArgs.value<DataPath>(k_DataObject_Key);
  auto newName = filterArgs.value<std::string>(k_NewName_Key);

  auto action = std::make_unique<RenameDataAction>(dataGroupPath, newName);

  OutputActions actions;
  actions.appendAction(std::move(action));
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RenameDataObject::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
