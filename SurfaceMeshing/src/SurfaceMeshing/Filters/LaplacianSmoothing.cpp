#include "LaplacianSmoothing.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string LaplacianSmoothing::name() const
{
  return FilterTraits<LaplacianSmoothing>::name.str();
}

//------------------------------------------------------------------------------
std::string LaplacianSmoothing::className() const
{
  return FilterTraits<LaplacianSmoothing>::className;
}

//------------------------------------------------------------------------------
Uuid LaplacianSmoothing::uuid() const
{
  return FilterTraits<LaplacianSmoothing>::uuid;
}

//------------------------------------------------------------------------------
std::string LaplacianSmoothing::humanName() const
{
  return "Laplacian Smoothing";
}

//------------------------------------------------------------------------------
std::vector<std::string> LaplacianSmoothing::defaultTags() const
{
  return {"#Surface Meshing", "#Smoothing"};
}

//------------------------------------------------------------------------------
Parameters LaplacianSmoothing::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_IterationSteps_Key, "Iteration Steps", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_Lambda_Key, "Default Lambda", "", 1.23345f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseTaubinSmoothing_Key, "Use Taubin Smoothing", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_MuFactor_Key, "Mu Factor", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_TripleLineLambda_Key, "Triple Line Lambda", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_QuadPointLambda_Key, "Quadruple Points Lambda", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfacePointLambda_Key, "Outer Points Lambda", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfaceTripleLineLambda_Key, "Outer Triple Line Lambda", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfaceQuadPointLambda_Key, "Outer Quadruple Points Lambda", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshNodeTypeArrayPath_Key, "Node Type", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseTaubinSmoothing_Key, k_MuFactor_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer LaplacianSmoothing::clone() const
{
  return std::make_unique<LaplacianSmoothing>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult LaplacianSmoothing::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pIterationStepsValue = filterArgs.value<int32>(k_IterationSteps_Key);
  auto pLambdaValue = filterArgs.value<float32>(k_Lambda_Key);
  auto pUseTaubinSmoothingValue = filterArgs.value<bool>(k_UseTaubinSmoothing_Key);
  auto pMuFactorValue = filterArgs.value<float32>(k_MuFactor_Key);
  auto pTripleLineLambdaValue = filterArgs.value<float32>(k_TripleLineLambda_Key);
  auto pQuadPointLambdaValue = filterArgs.value<float32>(k_QuadPointLambda_Key);
  auto pSurfacePointLambdaValue = filterArgs.value<float32>(k_SurfacePointLambda_Key);
  auto pSurfaceTripleLineLambdaValue = filterArgs.value<float32>(k_SurfaceTripleLineLambda_Key);
  auto pSurfaceQuadPointLambdaValue = filterArgs.value<float32>(k_SurfaceQuadPointLambda_Key);
  auto pSurfaceMeshNodeTypeArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshNodeTypeArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

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

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> LaplacianSmoothing::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pIterationStepsValue = filterArgs.value<int32>(k_IterationSteps_Key);
  auto pLambdaValue = filterArgs.value<float32>(k_Lambda_Key);
  auto pUseTaubinSmoothingValue = filterArgs.value<bool>(k_UseTaubinSmoothing_Key);
  auto pMuFactorValue = filterArgs.value<float32>(k_MuFactor_Key);
  auto pTripleLineLambdaValue = filterArgs.value<float32>(k_TripleLineLambda_Key);
  auto pQuadPointLambdaValue = filterArgs.value<float32>(k_QuadPointLambda_Key);
  auto pSurfacePointLambdaValue = filterArgs.value<float32>(k_SurfacePointLambda_Key);
  auto pSurfaceTripleLineLambdaValue = filterArgs.value<float32>(k_SurfaceTripleLineLambda_Key);
  auto pSurfaceQuadPointLambdaValue = filterArgs.value<float32>(k_SurfaceQuadPointLambda_Key);
  auto pSurfaceMeshNodeTypeArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshNodeTypeArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
