#include "FindDifferencesMap.hpp"

#include <vector>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

namespace complex
{
namespace
{
template <typename DataType>
Result<> validateArrayTypes(IFilter* filter, std::vector<IDataArray*> ptrArray)
{
  for(auto dataArray : ptrArray)
  {
    if(dynamic_cast<BoolArray*>()(dataArray))
    {
      std::string ss = fmt::format("Selected Attribute Arrays cannot be of type bool");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-90000, ss}})};
    }
    if(!dynamic_cast<DataArray<DataType>*>()(dataArray))
    {
      std::string ss = fmt::format("Selected Attribute Arrays must all be of the same type");
      return {nonstd::make_unexpected(std::vector<Error>{Error{-90001, ss}})};
    }
  }
  return {};
}

Result<> warnOnUnsignedTypes(IDataArray* ptr)
{
  if(dynamic_cast<UInt8Array*>(ptr))
  {
    std::string ss = fmt::format("Selected Attribute Arrays are of type uint8_t. Using unsigned integer types may result in underflow leading to extremely large values!");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-90004, ss}})};
  }
  if(dynamic_cast<UInt16Array*>(ptr))
  {
    std::string ss = fmt::format("Selected Attribute Arrays are of type uint16_t. Using unsigned integer types may result in underflow leading to extremely large values!");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-90005, ss}})};
  }
  if(dynamic_cast<UInt32Array*>(ptr))
  {
    std::string ss = fmt::format("Selected Attribute Arrays are of type uint32_t. Using unsigned integer types may result in underflow leading to extremely large values!");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-90006, ss}})};
  }
  if(dynamic_cast<UInt64Array*>(ptr))
  {
    std::string ss = fmt::format("Selected Attribute Arrays are of type uint64_t. Using unsigned integer types may result in underflow leading to extremely large values!");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-90007, ss}})};
  }
  return {};
}
} // namespace

std::string FindDifferencesMap::name() const
{
  return FilterTraits<FindDifferencesMap>::name;
}

std::string FindDifferencesMap::className() const
{
  return FilterTraits<FindDifferencesMap>::className;
}

Uuid FindDifferencesMap::uuid() const
{
  return FilterTraits<FindDifferencesMap>::uuid;
}

std::string FindDifferencesMap::humanName() const
{
  return "Find Differences Map";
}

Parameters FindDifferencesMap::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_DataPath_Key, "DataPath to remove", "DataPath to DataObject", DataPath{}));
  return params;
}

IFilter::UniquePointer FindDifferencesMap::clone() const
{
  return std::make_unique<FindDifferencesMap>();
}

IFilter::PreflightResult FindDifferencesMap::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto dataArrayPath = args.value<DataPath>(k_DataPath_Key);

  auto action = std::make_unique<CreateArrayAction>(dataArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> FindDifferencesMap::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
