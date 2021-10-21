#include "VMFindDistanceFieldFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string VMFindDistanceFieldFromTriangleGeometry::name() const
{
  return FilterTraits<VMFindDistanceFieldFromTriangleGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string VMFindDistanceFieldFromTriangleGeometry::className() const
{
  return FilterTraits<VMFindDistanceFieldFromTriangleGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid VMFindDistanceFieldFromTriangleGeometry::uuid() const
{
  return FilterTraits<VMFindDistanceFieldFromTriangleGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string VMFindDistanceFieldFromTriangleGeometry::humanName() const
{
  return "Find Distance Field from Triangle Geometry (VolumeMeshing)";
}

//------------------------------------------------------------------------------
std::vector<std::string> VMFindDistanceFieldFromTriangleGeometry::defaultTags() const
{
  return {"#Volume Meshing", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters VMFindDistanceFieldFromTriangleGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_DistanceFieldType_Key, "Distance Field Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreClosestTriangle_Key, "Store Closest Trianle", "", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleDataContainerName_Key, "Triangle Data Container", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageDataContainerName_Key, "Image Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_SignedDistanceFieldName_Key, "Signed Distance Field", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_ClosestTriangleName_Key, "Closest Triangle", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_StoreClosestTriangle_Key, k_ClosestTriangleName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VMFindDistanceFieldFromTriangleGeometry::clone() const
{
  return std::make_unique<VMFindDistanceFieldFromTriangleGeometry>();
}

//------------------------------------------------------------------------------
Result<OutputActions> VMFindDistanceFieldFromTriangleGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDistanceFieldTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceFieldType_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pStoreClosestTriangleValue = filterArgs.value<bool>(k_StoreClosestTriangle_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pImageDataContainerNameValue = filterArgs.value<DataPath>(k_ImageDataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pSignedDistanceFieldNameValue = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldName_Key);
  auto pClosestTriangleNameValue = filterArgs.value<StringParameter::ValueType>(k_ClosestTriangleName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<VMFindDistanceFieldFromTriangleGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> VMFindDistanceFieldFromTriangleGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDistanceFieldTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceFieldType_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pStoreClosestTriangleValue = filterArgs.value<bool>(k_StoreClosestTriangle_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pImageDataContainerNameValue = filterArgs.value<DataPath>(k_ImageDataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pSignedDistanceFieldNameValue = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldName_Key);
  auto pClosestTriangleNameValue = filterArgs.value<StringParameter::ValueType>(k_ClosestTriangleName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
