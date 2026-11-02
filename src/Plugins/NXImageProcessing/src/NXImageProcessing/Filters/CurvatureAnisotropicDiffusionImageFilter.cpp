#include "CurvatureAnisotropicDiffusionImageFilter.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

using namespace nx::core;

namespace nx::core
{
std::string CurvatureAnisotropicDiffusionImageFilter::name() const
{
  return FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::name;
}
std::string CurvatureAnisotropicDiffusionImageFilter::className() const
{
  return FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::className;
}
Uuid CurvatureAnisotropicDiffusionImageFilter::uuid() const
{
  return FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::uuid;
}
std::string CurvatureAnisotropicDiffusionImageFilter::humanName() const
{
  return "Curvature Anisotropic Diffusion Image Filter";
}
std::vector<std::string> CurvatureAnisotropicDiffusionImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Smoothing", "AnisotropicDiffusion", "PDE"};
}

Parameters CurvatureAnisotropicDiffusionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "Time Step", "The time step to be used for each iteration.", 0.0625));
  params.insert(std::make_unique<Float64Parameter>(k_ConductanceParameter_Key, "Conductance Parameter",
                                                   "The conductance parameter controls the sensitivity of the conductance term in the basic anisotropic diffusion equation", 3.0));
  params.insert(std::make_unique<UInt32Parameter>(k_ConductanceScalingUpdateInterval_Key, "Conductance Scaling Update Interval", "The interval between conductance updates.", 1u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "Number Of Iterations",
                                                  "Specifies the number of iterations (time-step updates) that the solver will perform to produce a solution image", 5u));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetFloatingScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}

IFilter::VersionType CurvatureAnisotropicDiffusionImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer CurvatureAnisotropicDiffusionImageFilter::clone() const
{
  return std::make_unique<CurvatureAnisotropicDiffusionImageFilter>();
}

IFilter::PreflightResult CurvatureAnisotropicDiffusionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);
  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);

  // requireScalar=true: the conductance term is defined on scalar images. The output is SameAsInput
  // (type-preserving). FloatingScalar admits only float32/float64 (the legacy FloatingScalarPixelIdTypeList) --
  // the exp()-based conductance term requires a signed, unbounded-precision pixel type (integer input is rejected).
  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightImageFilter<ImageProcessing::FloatingScalar, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);

  // ITK's AnisotropicDiffusionImageFilter::InitializeIteration WARNS (never throws) when TimeStep exceeds the CFL
  // stability bound. Only append the check once the input/geometry above are confirmed valid (imageGeomPath is
  // guaranteed resolvable by PreflightImageFilter having succeeded).
  if(resultOutputActions.valid())
  {
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
    ImageProcessing::AppendUnstableTimeStepWarning(resultOutputActions, timeStep, imageGeom.getSpacing(), imageGeom.getDimensions(), ImageProcessing::k_AnisotropicDiffusionUnstableTimeStep);
  }

  return {std::move(resultOutputActions)};
}

Result<> CurvatureAnisotropicDiffusionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto conductanceParameter = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto conductanceScalingUpdateInterval = filterArgs.value<uint32>(k_ConductanceScalingUpdateInterval_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  // Aggregate-init in the functor's declared member order (m_K, conductance, interval, gradientMagnitudeFixed,
  // fixedAvgGradMagSq): m_K starts at 0 (the driver's very first UpdateGlobalConductance call -- iter==0 -- always
  // recomputes it before it is ever read by computeUpdate); gradientMagnitudeFixed/fixedAvgGradMagSq keep their
  // ITK-fidelity defaults (false/0.0) since this filter (matching the legacy wrapper) never exposes them.
  return ImageProcessing::ExecuteFiniteDifferenceImageFilter<ImageProcessing::detail::CurvatureAnisoFn, ImageProcessing::FloatingScalar>(
      dataStructure, imageGeomPath, selectedInputArray, outputArrayPath,
      ImageProcessing::detail::CurvatureAnisoFn{/*m_K=*/0.0, /*conductance=*/conductanceParameter,
                                                /*interval=*/conductanceScalingUpdateInterval},
      timeStep, numberOfIterations, shouldCancel, messageHandler);
}
} // namespace nx::core
