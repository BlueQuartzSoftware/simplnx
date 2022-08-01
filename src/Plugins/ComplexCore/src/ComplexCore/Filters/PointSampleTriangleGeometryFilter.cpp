#include "PointSampleTriangleGeometryFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
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
  return {"#ComplexCore", "#Geometry", "#TriangleGeometry", "#Resample"};
}

//------------------------------------------------------------------------------
Parameters PointSampleTriangleGeometryFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  // params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_SamplesNumberType_Key, "Source for Number of Samples", "", 0, ChoicesParameter::Choices{"Manual", "Other Geometry"}));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfSamples_Key, "Number of Sample Points", "The number of sample points to use", 1000));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry to Sample", "The complete path to the triangle Geometry from which to sample", DataPath{}));
  // params.insert(std::make_unique<DataPathSelectionParameter>(k_ParentGeometry_Key, "Source Geometry for Number of Sample Points", "", DataPath{}, true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Whether to use a boolean mask array to ignore certain Trianlges flagged as false from the sampling algorithm", false));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleAreasArrayPath_Key, "Face Areas", "The complete path to the array specifying the area of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The complete path to the array specifying if the Face can be sampled, if Use Mask is checked", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Face Attribute Arrays to Transfer",
                                                               "The paths to the Face Attribute Arrays to transfer to the created Vertex Geometry where the mask is false, if Use Mask is checked",
                                                               MultiArraySelectionParameter::ValueType{}, complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Objects"});

  // params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexParentGroup_Key, "Created Vertex Geometry Parent [Data Group]", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexGeometryPath_Key, "Vertex Geometry Name",
                                                             "The complete path to the DataGroup holding the Vertex Geometry that represents the sampling points", DataPath({"[Vertex Geometry]"})));
  params.insert(
      std::make_unique<ArrayCreationParameter>(k_VertexDataGroupPath_Key, "Vertex Data", "The complete path to the Array for the Vertex Geometry", DataPath({"[Vertex Geometry]", "Vertex Data"})));

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
IFilter::PreflightResult PointSampleTriangleGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel) const
{

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNumberOfSamples = filterArgs.value<int32>(k_NumberOfSamples_Key);
  auto pUseMask = filterArgs.value<bool>(k_UseMask_Key);
  auto pTriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  auto pTriangleAreasArrayPath = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  auto pMaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pVertexGeometryDataPath = filterArgs.value<DataPath>(k_VertexGeometryPath_Key);
  auto pVertexGroupDataPath = filterArgs.value<DataPath>(k_VertexDataGroupPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions = {};

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Create the Vertex Geometry action and store it
  {
    auto createVertexGeometryAction = std::make_unique<CreateVertexGeometryAction>(pVertexGeometryDataPath, pNumberOfSamples);
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

  // Collect all the errors
  std::vector<Error> errors;

  // Ensure that if pMaskValue is TRUE that the Mask Path is valid
  if(pUseMask)
  {
    const DataObject* maskArray = dataStructure.getData(pMaskArrayPath);
    if(nullptr == maskArray)
    {
      Error result = {-500, fmt::format("'Use Mask Array' is selected but the DataPath '{}' does not exist. Please ensure the mask array exists in the DataStructure.", pMaskArrayPath.toString())};
      errors.push_back(result);
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  // Return both the resultOutputActions and the preflightUpdatedValues  LinkGeometryDataFilter via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> PointSampleTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/

  PointSampleTriangleGeometryInputs inputs;

  inputs.pNumberOfSamples = filterArgs.value<int32>(k_NumberOfSamples_Key);
  inputs.pUseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputs.pTriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  inputs.pTriangleAreasArrayPath = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  inputs.pMaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputs.pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputs.pVertexGeometryPath = filterArgs.value<DataPath>(k_VertexGeometryPath_Key);
  inputs.pVertexGroupDataPath = filterArgs.value<DataPath>(k_VertexDataGroupPath_Key);

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

  return PointSampleTriangleGeometry(dataStructure, &inputs, shouldCancel, messageHandler)();
}
} // namespace complex
