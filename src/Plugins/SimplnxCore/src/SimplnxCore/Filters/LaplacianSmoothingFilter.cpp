#include "LaplacianSmoothingFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/LaplacianSmoothing.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
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
  return {className(), "Surface Meshing", "Smoothing", "Triangle Geometry"};
}

//------------------------------------------------------------------------------
Parameters LaplacianSmoothingFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<Int32Parameter>(
      k_IterationSteps_Key, "Iteration Steps",
      "Number of iteration steps to perform. More steps causes more smoothing but will also cause the volume to shrink more. Increasing this number too high may cause collapse of points!", 25));
  params.insert(std::make_unique<Float32Parameter>(k_Lambda_Key, "Default Lambda",
                                                   "Value of λ to apply to general internal nodes that are not triple lines, quadruple points or on the surface of the volume", 0.2F));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseTaubinSmoothing_Key, "Use Taubin Smoothing", "Use Taubin's Lambda-Mu algorithm.", false));
  params.insert(std::make_unique<Float32Parameter>(k_MuFactor_Key, "Mu Factor",
                                                   "A value that is multiplied by Lambda the result of which is the mu in Taubin's paper. The value should be a negative value.", 0.2F));
  params.insert(std::make_unique<Float32Parameter>(k_TripleLineLambda_Key, "Triple Line Lambda", "Value of λ to apply to nodes designated as triple line nodes.", 0.1f));
  params.insert(std::make_unique<Float32Parameter>(k_QuadPointLambda_Key, "Quadruple Points Lambda", "Value of λ to apply to nodes designated as quadruple points.", 0.1f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfacePointLambda_Key, "Outer Points Lambda", "The value of λ to apply to nodes that lie on the outer surface of the volume", 0.0f));
  params.insert(std::make_unique<Float32Parameter>(k_SurfaceTripleLineLambda_Key, "Outer Triple Line Lambda", "Value of λ for triple lines that lie on the outer surface of the volume", 0.0f));
  params.insert(
      std::make_unique<Float32Parameter>(k_SurfaceQuadPointLambda_Key, "Outer Quadruple Points Lambda", "Value of λ for the quadruple Points that lie on the outer surface of the volume.", 0.0f));

  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry"});
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry",
                                                             "The complete path to the surface mesh Geometry for which to apply Laplacian smoothing", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle, IGeometry::Type::Tetrahedral}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshNodeTypeArrayPath_Key, "Node Type", "The complete path to the array specifying the type of node in the Geometry", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

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
IFilter::PreflightResult LaplacianSmoothingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
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

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> LaplacianSmoothingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  nx::core::LaplacianSmoothingInputValues inputValues;

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
  return LaplacianSmoothing(dataStructure, &inputValues, shouldCancel, messageHandler)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_IterationStepsKey = "IterationSteps";
constexpr StringLiteral k_LambdaKey = "Lambda";
constexpr StringLiteral k_UseTaubinSmoothingKey = "UseTaubinSmoothing";
constexpr StringLiteral k_MuFactorKey = "MuFactor";
constexpr StringLiteral k_TripleLineLambdaKey = "TripleLineLambda";
constexpr StringLiteral k_QuadPointLambdaKey = "QuadPointLambda";
constexpr StringLiteral k_SurfacePointLambdaKey = "SurfacePointLambda";
constexpr StringLiteral k_SurfaceTripleLineLambdaKey = "SurfaceTripleLineLambda";
constexpr StringLiteral k_SurfaceQuadPointLambdaKey = "SurfaceQuadPointLambda";
constexpr StringLiteral k_SurfaceMeshNodeTypeArrayPathKey = "SurfaceMeshNodeTypeArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> LaplacianSmoothingFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = LaplacianSmoothingFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_IterationStepsKey, k_IterationSteps_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_LambdaKey, k_Lambda_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseTaubinSmoothingKey, k_UseTaubinSmoothing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MuFactorKey, k_MuFactor_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_TripleLineLambdaKey, k_TripleLineLambda_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_QuadPointLambdaKey, k_QuadPointLambda_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_SurfacePointLambdaKey, k_SurfacePointLambda_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_SurfaceTripleLineLambdaKey, k_SurfaceTripleLineLambda_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_SurfaceQuadPointLambdaKey, k_SurfaceQuadPointLambda_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshNodeTypeArrayPathKey, k_SurfaceMeshNodeTypeArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
