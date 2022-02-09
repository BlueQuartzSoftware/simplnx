#include "VMFindDistanceFieldFromTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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
IFilter::PreflightResult VMFindDistanceFieldFromTriangleGeometry::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pDistanceFieldTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceFieldType_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pStoreClosestTriangleValue = filterArgs.value<bool>(k_StoreClosestTriangle_Key);
  auto pTriangleDataContainerNameValue = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  auto pImageDataContainerNameValue = filterArgs.value<DataPath>(k_ImageDataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto pSignedDistanceFieldNameValue = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldName_Key);
  auto pClosestTriangleNameValue = filterArgs.value<StringParameter::ValueType>(k_ClosestTriangleName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pImageDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> VMFindDistanceFieldFromTriangleGeometry::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
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
