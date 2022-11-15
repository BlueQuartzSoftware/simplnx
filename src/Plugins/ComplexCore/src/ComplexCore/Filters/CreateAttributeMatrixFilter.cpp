#include "CreateAttributeMatrixFilter.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{
std::string CreateAttributeMatrixFilter::name() const
{
  return FilterTraits<CreateAttributeMatrixFilter>::name;
}

std::string CreateAttributeMatrixFilter::className() const
{
  return FilterTraits<CreateAttributeMatrixFilter>::className;
}

Uuid CreateAttributeMatrixFilter::uuid() const
{
  return FilterTraits<CreateAttributeMatrixFilter>::uuid;
}

std::string CreateAttributeMatrixFilter::humanName() const
{
  return "Create Attribute Matrix";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateAttributeMatrixFilter::defaultTags() const
{
  return {"#Core", "#Generation", "#AttributeMatrix", "#Create"};
}

Parameters CreateAttributeMatrixFilter::parameters() const
{
  Parameters params;


  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataObjectPath, "DataObject Path", "The complete path to the Attribute Matrix being created", DataPath{}));
  DynamicTableInfo tableInfo;
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "DIM {}"));

  params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Attribute Matrix Dimensions (Slowest to Fastest Dimensions)", "Slowest to Fastest Dimensions", tableInfo));
  return params;
}

IFilter::UniquePointer CreateAttributeMatrixFilter::clone() const
{
  return std::make_unique<CreateAttributeMatrixFilter>();
}

IFilter::PreflightResult CreateAttributeMatrixFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  DataPath dataObjectPath = args.value<DataPath>(k_DataObjectPath);
  auto tableData = args.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);

  const auto& rowData = tableData.at(0);
  std::vector<usize> tupleDims;
  tupleDims.reserve(rowData.size());
  for(auto floatValue : rowData)
  {
    tupleDims.push_back(static_cast<usize>(floatValue));
  }

  auto action = std::make_unique<CreateAttributeMatrixAction>(dataObjectPath, tupleDims);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CreateAttributeMatrixFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
