#include "LaplacianSmoothing.hpp"

#include "complex/DataStructure/DataPath.hpp"
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
IFilter::PreflightResult LaplacianSmoothing::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
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

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<LaplacianSmoothingAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
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
