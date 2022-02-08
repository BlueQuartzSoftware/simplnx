#include "LaplacianSmoothingFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ComplexCore/Filters/Algorithms/LaplacianSmoothing.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string LaplacianSmoothingFilter::name() const
{
  return FilterTraits<LaplacianSmoothingFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string LaplacianSmoothingFilter::className() const
{
  return FilterTraits<LaplacianSmoothingFilter>::className;
}

//------------------------------------------------------------------------------
Uuid LaplacianSmoothingFilter::uuid() const
{
  return FilterTraits<LaplacianSmoothingFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string LaplacianSmoothingFilter::humanName() const
{
  return "Laplacian Smoothing";
}

//------------------------------------------------------------------------------
std::vector<std::string> LaplacianSmoothingFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Smoothing", "#Triangle Geometry"};
}

//------------------------------------------------------------------------------
Parameters LaplacianSmoothingFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry", "", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Triangle, AbstractGeometry::Type::Tetrahedral}));
  params.insertSeparator(Parameters::Separator{"Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshNodeTypeArrayPath_Key, "Node Type", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Smoothing Values"});

  params.insert(std::make_unique<Int32Parameter>(k_IterationSteps_Key, "Iteration Steps", "", 25));
  params.insert(std::make_unique<Float32Parameter>(k_Lambda_Key, "Default Lambda", "", 0.2F));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseTaubinSmoothing_Key, "Use Taubin Smoothing", "", false));
  params.insert(std::make_unique<Float32Parameter>(k_MuFactor_Key, "Mu Factor", "", 0.2F));
  params.insert(std::make_unique<Float32Parameter>(k_TripleLineLambda_Key, "Triple Line Lambda", "", 0.1f));
  params.insert(std::make_unique<Float32Parameter>(k_QuadPointLambda_Key, "Quadruple Points Lambda", "", 0.1f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfacePointLambda_Key, "Outer Points Lambda", "", 0.0f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfaceTripleLineLambda_Key, "Outer Triple Line Lambda", "", 0.0f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfaceQuadPointLambda_Key, "Outer Quadruple Points Lambda", "", 0.0f));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseTaubinSmoothing_Key, k_MuFactor_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer LaplacianSmoothingFilter::clone() const
{
  return std::make_unique<LaplacianSmoothingFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult LaplacianSmoothingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
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

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> LaplacianSmoothingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  complex::LaplacianSmoothingInputValues inputValues;

  inputValues.pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);
  inputValues.pIterationSteps = filterArgs.value<int32>(k_IterationSteps_Key);
  inputValues.pLambda = filterArgs.value<float32>(k_Lambda_Key);
  inputValues.pUseTaubinSmoothing = filterArgs.value<bool>(k_UseTaubinSmoothing_Key);
  inputValues.pMuFactor = filterArgs.value<float32>(k_MuFactor_Key);
  inputValues.pTripleLineLambda = filterArgs.value<float32>(k_TripleLineLambda_Key);
  inputValues.pQuadPointLambda = filterArgs.value<float32>(k_QuadPointLambda_Key);
  inputValues.pSurfacePointLambda = filterArgs.value<float32>(k_SurfacePointLambda_Key);
  inputValues.pSurfaceTripleLineLambda = filterArgs.value<float32>(k_SurfaceTripleLineLambda_Key);
  inputValues.pSurfaceQuadPointLambda = filterArgs.value<float32>(k_SurfaceQuadPointLambda_Key);
  inputValues.pSurfaceMeshNodeTypeArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshNodeTypeArrayPath_Key);

  // Let the Algorithm instance do the work
  return LaplacianSmoothing(dataStructure, &inputValues, this, messageHandler)();
}
} // namespace complex
