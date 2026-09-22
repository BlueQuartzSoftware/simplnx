#include "CurvatureFlowImageFilter.hpp"

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
std::string CurvatureFlowImageFilter::name() const
{
  return FilterTraits<CurvatureFlowImageFilter>::name;
}
std::string CurvatureFlowImageFilter::className() const
{
  return FilterTraits<CurvatureFlowImageFilter>::className;
}
Uuid CurvatureFlowImageFilter::uuid() const
{
  return FilterTraits<CurvatureFlowImageFilter>::uuid;
}
std::string CurvatureFlowImageFilter::humanName() const
{
  return "Curvature Flow Image Filter";
}
std::vector<std::string> CurvatureFlowImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Smoothing", "CurvatureFlow", "PDE"};
}

Parameters CurvatureFlowImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "Time Step", "The timestep between iteration updates", 0.05));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "Number Of Iterations", "The number of update iterations ", 5u));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}

IFilter::VersionType CurvatureFlowImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer CurvatureFlowImageFilter::clone() const
{
  return std::make_unique<CurvatureFlowImageFilter>();
}

IFilter::PreflightResult CurvatureFlowImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: curvature flow is defined on scalar images. The output is SameAsInput (type-preserving); an
  // input type T produces a T-typed output array. AllNumeric admits every scalar type (integer AND floating point --
  // the legacy ScalarPixelIdTypeList), matching the legacy ITK filter, which has no minimum-dimension requirement.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}

Result<> CurvatureFlowImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  return ImageProcessing::ExecuteFiniteDifferenceImageFilter<ImageProcessing::detail::CurvatureFlowFn, ImageProcessing::AllNumeric>(
      dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, ImageProcessing::detail::CurvatureFlowFn{}, timeStep, numberOfIterations, shouldCancel, messageHandler);
}
} // namespace nx::core
