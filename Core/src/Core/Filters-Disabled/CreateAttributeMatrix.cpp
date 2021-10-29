#include "CreateAttributeMatrix.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateAttributeMatrix::name() const
{
  return FilterTraits<CreateAttributeMatrix>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateAttributeMatrix::className() const
{
  return FilterTraits<CreateAttributeMatrix>::className;
}

//------------------------------------------------------------------------------
Uuid CreateAttributeMatrix::uuid() const
{
  return FilterTraits<CreateAttributeMatrix>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateAttributeMatrix::humanName() const
{
  return "Create Attribute Matrix";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateAttributeMatrix::defaultTags() const
{
  return {"#Core", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters CreateAttributeMatrix::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_AttributeMatrixType_Key, "Attribute Matrix Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  /*[x]*/ params.insert(std::make_unique<DynamicTableFilterParameter>(k_TupleDimensions_Key, "Tuple Dimensions", "", {}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedAttributeMatrix_Key, "Created Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateAttributeMatrix::clone() const
{
  return std::make_unique<CreateAttributeMatrix>();
}

//------------------------------------------------------------------------------
Result<OutputActions> CreateAttributeMatrix::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pAttributeMatrixTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_AttributeMatrixType_Key);
  auto pTupleDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_TupleDimensions_Key);
  auto pCreatedAttributeMatrixValue = filterArgs.value<DataPath>(k_CreatedAttributeMatrix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateAttributeMatrixAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CreateAttributeMatrix::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pAttributeMatrixTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_AttributeMatrixType_Key);
  auto pTupleDimensionsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_TupleDimensions_Key);
  auto pCreatedAttributeMatrixValue = filterArgs.value<DataPath>(k_CreatedAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
