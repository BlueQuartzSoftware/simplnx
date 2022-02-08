#include "RenameAttributeArray.hpp"

#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <string>

namespace
{
constexpr complex::int32 k_ARRAY_NOT_FOUND = -700;
constexpr complex::int32 k_COULD_NOT_RENAME = -701;
} // namespace

namespace complex
{

std::string RenameAttributeArray::name() const
{
  return FilterTraits<RenameAttributeArray>::name;
}

std::string RenameAttributeArray::className() const
{
  return FilterTraits<RenameAttributeArray>::className;
}

Uuid RenameAttributeArray::uuid() const
{
  return FilterTraits<RenameAttributeArray>::uuid;
}

std::string RenameAttributeArray::humanName() const
{
  return "Create Data Array";
}

Parameters RenameAttributeArray::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<ArraySelectionParameter>(k_ArrayPath_Key, "Renamed Array", "Array to be renamed", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_NewName_Key, "New Name", "New name to assign to the target DataArray", "Renamed Array"));
  return params;
}

IFilter::UniquePointer RenameAttributeArray::clone() const
{
  return std::make_unique<RenameAttributeArray>();
}

IFilter::PreflightResult RenameAttributeArray::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto arrayPath = filterArgs.value<DataPath>(k_ArrayPath_Key);
  auto newName = filterArgs.value<std::string>(k_NewName_Key);

  auto& dataArray = dataStructure.getDataRefAs<IDataArray>(arrayPath);
  if(!dataArray.canRename(newName))
  {
    std::string ss = fmt::format("Could not rename DataArray at path '{}' to '{}'", arrayPath.toString(), newName);
    return {MakeErrorResult<OutputActions>(::k_COULD_NOT_RENAME, ss)};
  }

  OutputActions actions;
  return {std::move(actions)};
}

Result<> RenameAttributeArray::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto arrayPath = args.value<DataPath>(k_ArrayPath_Key);
  auto newName = args.value<std::string>(k_NewName_Key);

  auto& dataArray = data.getDataRefAs<IDataArray>(arrayPath);
  dataArray.rename(newName);

  return {};
}
} // namespace complex
