#include "SumProjectionImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ProjectionReducers.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string SumProjectionImageFilter::name() const
{
  return FilterTraits<SumProjectionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string SumProjectionImageFilter::className() const
{
  return FilterTraits<SumProjectionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SumProjectionImageFilter::uuid() const
{
  return FilterTraits<SumProjectionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SumProjectionImageFilter::humanName() const
{
  return "Sum Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> SumProjectionImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Sum", "Projection", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters SumProjectionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "Projection Dimension", "The axis to project along: 0=X, 1=Y, 2=Z.", 0u));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In-Place", "Performs the projection in-place for the given Image Geometry", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputImageGeomName_Key, "Created Image Geometry", "The name of the projected geometry", "Projected Image"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  params.linkParameters(k_RemoveOriginalGeometry_Key, k_OutputImageGeomName_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SumProjectionImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SumProjectionImageFilter::clone() const
{
  return std::make_unique<SumProjectionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SumProjectionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto performInPlace = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto outputGeomName = filterArgs.value<std::string>(k_OutputImageGeomName_Key);

  // The sum of an integer column overflows narrow integer types, so the legacy ITK filter emits a double
  // (Float64) output regardless of input type. The façade is therefore instantiated with the AllNumeric
  // input policy (any scalar numeric input) and the AlwaysFloat64 OutTypeMap (Float64 output); the
  // execute façade below MUST use the same pair, or the allocated and written output types disagree. The
  // scalar (single-component) and tuple-consistency checks are performed inside PreflightAxisProjection.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightAxisProjection<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(
      dataStructure, imageGeomPath, selectedInputArray, static_cast<usize>(projectionDimension), performInPlace, outputGeomName, outputArrayName);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> SumProjectionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto outputGeomName = filterArgs.value<std::string>(k_OutputImageGeomName_Key);
  auto performInPlace = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  // Resolve the output array's location via the shared helper so preflight (which allocated it) and this
  // execute (which writes it) stay byte-identical. For an in-place run this is the temporary geometry the
  // deferred swap renames afterward; the original geometry is still intact here.
  const DataPath outputArrayPath = ImageProcessing::ProjectionOutputArrayPath(dataStructure, imageGeomPath, performInPlace, outputGeomName, outputArrayName);

  const ImageProcessing::SumReduce reduce{};
  return ImageProcessing::ExecuteAxisProjectionImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat64>(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath,
                                                                                                                        static_cast<usize>(projectionDimension), reduce, shouldCancel, messageHandler);
}
} // namespace nx::core
