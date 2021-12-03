#include "PointSampleTriangleGeometryFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "ComplexCore/Filters/Algorithms/PointSampleTriangleGeometry.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string PointSampleTriangleGeometryFilter::name() const
{
  return FilterTraits<PointSampleTriangleGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string PointSampleTriangleGeometryFilter::className() const
{
  return FilterTraits<PointSampleTriangleGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid PointSampleTriangleGeometryFilter::uuid() const
{
  return FilterTraits<PointSampleTriangleGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string PointSampleTriangleGeometryFilter::humanName() const
{
  return "Point Sample Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> PointSampleTriangleGeometryFilter::defaultTags() const
{
  return {"#DREAM3D Review", "#Geometry", "#TriangleGeometry", "#Resample"};
}

//------------------------------------------------------------------------------
Parameters PointSampleTriangleGeometryFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  // params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SamplesNumberType_Key, "Source for Number of Samples", "", 0, ChoicesParameter::Choices{"Manual", "Other Geometry"}));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfSamples_Key, "Number of Sample Points", "", 1000));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry to Sample", "", DataPath{}));
  // params.insert(std::make_unique<DataPathSelectionParameter>(k_ParentGeometry_Key, "Source Geometry for Number of Sample Points", "", DataPath{}, true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleAreasArrayPath_Key, "Face Areas", "", DataPath{}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}, true));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Face Attribute Arrays to Transfer", "", MultiArraySelectionParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Created Objects"});

  params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexParentGroup_Key, "Created Vertex Geometry Parent [Data Group]", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_VertexGeometryName_Key, "Vertex Geometry Name", "", "[Vertex Geometry]"));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexData_DataPath_Key, "Vertex Data", "", DataPath{}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  //  params.linkParameters(k_SamplesNumberType_Key, k_NumberOfSamples_Key, 0);
  //  params.linkParameters(k_SamplesNumberType_Key, k_ParentGeometry_Key, 1);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PointSampleTriangleGeometryFilter::clone() const
{
  return std::make_unique<PointSampleTriangleGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PointSampleTriangleGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  // auto pSamplesNumberType = filterArgs.value<ChoicesParameter::ValueType>(k_SamplesNumberType_Key);
  auto pNumberOfSamples = filterArgs.value<int32>(k_NumberOfSamples_Key);
  auto pUseMask = filterArgs.value<bool>(k_UseMask_Key);
  auto pTriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  // auto pParentGeometry = filterArgs.value<DataPath>(k_ParentGeometry_Key);
  auto pTriangleAreasArrayPath = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  auto pMaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pVertexGeomParentGroup = filterArgs.value<DataPath>(k_VertexParentGroup_Key);
  auto pVertexGeometryName = filterArgs.value<StringParameter::ValueType>(k_VertexGeometryName_Key);
  auto pVertexGroupDataPath = filterArgs.value<DataPath>(k_VertexData_DataPath_Key);

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

  DataPath geometryDataPath = pVertexGeomParentGroup.createChildPath(pVertexGeometryName);
  // Create the Vertex Geometry action and store it
  {
    auto createVertexGeometryAction = std::make_unique<CreateVertexGeometryAction>(geometryDataPath, pNumberOfSamples);
    resultOutputActions.value().actions.push_back(std::move(createVertexGeometryAction));
  }

  // Create  Vertex Data Group action and store it
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pVertexGroupDataPath);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  // Create all the target DataArray based on the Selected Node Arrays
  for(const auto& selectedDataPath : pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = pVertexGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    auto createArrayAction = std::make_unique<CopyArrayInstanceAction>(selectedDataPath, createdDataPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Ensure that if pMaskValue is TRUE that the Mask Path is valid
  if(pUseMask)
  {
    const DataObject* maskArray = dataStructure.getData(pMaskArrayPath);
    if(nullptr == maskArray)
    {
      Error result = {-500, fmt::format("'Use Mask Array' is selected but the DataPath '{}' does not exist. Please ensure the mask array exists in the DataStructure.", pMaskArrayPath.toString())};
      resultOutputActions.errors().push_back(result);
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> PointSampleTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/

  PointSampleTriangleGeometryInputs inputs;

  // inputs.pSamplesNumberType = filterArgs.value<ChoicesParameter::ValueType>(k_SamplesNumberType_Key);
  inputs.pNumberOfSamples = filterArgs.value<int32>(k_NumberOfSamples_Key);
  inputs.pUseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputs.pTriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  // inputs.pParentGeometry = filterArgs.value<DataPath>(k_ParentGeometry_Key);
  inputs.pTriangleAreasArrayPath = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  inputs.pMaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputs.pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputs.pVertexGeomParentGroup = filterArgs.value<DataPath>(k_VertexParentGroup_Key);
  inputs.pVertexGeometryName = filterArgs.value<StringParameter::ValueType>(k_VertexGeometryName_Key);
  inputs.pVertexGroupDataPath = filterArgs.value<DataPath>(k_VertexData_DataPath_Key);

  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputs.pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = inputs.pVertexGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    createdDataPaths.push_back(createdDataPath);
  }
  inputs.pCreatedDataArrayPaths = createdDataPaths;

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return PointSampleTriangleGeometry(dataStructure, &inputs, this, messageHandler)();
}
} // namespace complex
