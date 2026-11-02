#include "MinMaxCurvatureFlowImageFilter.hpp"

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
std::string MinMaxCurvatureFlowImageFilter::name() const
{
  return FilterTraits<MinMaxCurvatureFlowImageFilter>::name;
}
std::string MinMaxCurvatureFlowImageFilter::className() const
{
  return FilterTraits<MinMaxCurvatureFlowImageFilter>::className;
}
Uuid MinMaxCurvatureFlowImageFilter::uuid() const
{
  return FilterTraits<MinMaxCurvatureFlowImageFilter>::uuid;
}
std::string MinMaxCurvatureFlowImageFilter::humanName() const
{
  return "Min Max Curvature Flow Image Filter";
}
std::vector<std::string> MinMaxCurvatureFlowImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Smoothing", "CurvatureFlow", "MinMax", "PDE"};
}

Parameters MinMaxCurvatureFlowImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "Time Step", "The timestep between iteration updates", 0.05));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "Number Of Iterations", "The number of update iterations ", 5u));
  params.insert(std::make_unique<Int32Parameter>(k_StencilRadius_Key, "Stencil Radius", "Set/Get the stencil radius.", 2));

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

IFilter::VersionType MinMaxCurvatureFlowImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer MinMaxCurvatureFlowImageFilter::clone() const
{
  return std::make_unique<MinMaxCurvatureFlowImageFilter>();
}

IFilter::PreflightResult MinMaxCurvatureFlowImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: min/max curvature flow is defined on scalar images. The output is SameAsInput
  // (type-preserving). FloatingScalar admits only float32/float64 (the legacy FloatingScalarPixelIdTypeList) --
  // unlike CurvatureFlow, the min/max threshold comparison requires a floating-point pixel type.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::FloatingScalar, ImageProcessing::SameAsInput>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}

Result<> MinMaxCurvatureFlowImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);
  auto stencilRadius = filterArgs.value<int32>(k_StencilRadius_Key);

  // itkMinMaxCurvatureFlowFunction::SetStencilRadius clamps any value <= 1 (including 0 or negative) up to 1 --
  // replicated here so an out-of-range user value matches live ITK's behavior exactly, not radius()==0 (a
  // single-plane, unwindowed neighborhood that would silently diverge from ITK's minimum radius-1 ball).
  const int64 clampedStencilRadius = (stencilRadius > 1) ? static_cast<int64>(stencilRadius) : int64{1};

  return ImageProcessing::ExecuteFiniteDifferenceImageFilter<ImageProcessing::detail::MinMaxCurvatureFlowFn, ImageProcessing::FloatingScalar>(
      dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, ImageProcessing::detail::MinMaxCurvatureFlowFn{clampedStencilRadius}, timeStep, numberOfIterations, shouldCancel,
      messageHandler);
}
} // namespace nx::core
