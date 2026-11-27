#include "ZeroCrossingImageFilter.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

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
std::string ZeroCrossingImageFilter::name() const
{
  return FilterTraits<ZeroCrossingImageFilter>::name;
}
std::string ZeroCrossingImageFilter::className() const
{
  return FilterTraits<ZeroCrossingImageFilter>::className;
}
Uuid ZeroCrossingImageFilter::uuid() const
{
  return FilterTraits<ZeroCrossingImageFilter>::uuid;
}
std::string ZeroCrossingImageFilter::humanName() const
{
  return "Zero Crossing Image Filter";
}
std::vector<std::string> ZeroCrossingImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "ZeroCrossing", "ImageFeature", "EdgeDetection"};
}

Parameters ZeroCrossingImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt8ParameterType>(k_ForegroundValue_Key, "Foreground Value", "Set/Get the label value for zero-crossing pixels.", 1u));
  params.insert(std::make_unique<UInt8ParameterType>(k_BackgroundValue_Key, "Background Value", "Set/Get the label value for non-zero-crossing pixels.", 0u));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          nx::core::ImageProcessing::GetSignedScalarTypes(), ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}

IFilter::VersionType ZeroCrossingImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointerType ZeroCrossingImageFilter::clone() const
{
  return std::make_unique<ZeroCrossingImageFilter>();
}

IFilter::PreflightResult ZeroCrossingImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AlwaysUInt8Type>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}

Result<> ZeroCrossingImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto foregroundValue = filterArgs.value<uint8>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<uint8>(k_BackgroundValue_Key);

  return ImageProcessing::ExecuteZeroCrossingImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, foregroundValue, backgroundValue, shouldCancel, messageHandler);
}
} // namespace nx::core
