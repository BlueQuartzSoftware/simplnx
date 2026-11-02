#include "MorphologicalWatershedFromMarkersImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

#include <fmt/core.h>

#include <optional>

using namespace nx::core;

namespace nx::core
{
std::string MorphologicalWatershedFromMarkersImageFilter::name() const
{
  return FilterTraits<MorphologicalWatershedFromMarkersImageFilter>::name;
}
std::string MorphologicalWatershedFromMarkersImageFilter::className() const
{
  return FilterTraits<MorphologicalWatershedFromMarkersImageFilter>::className;
}
Uuid MorphologicalWatershedFromMarkersImageFilter::uuid() const
{
  return FilterTraits<MorphologicalWatershedFromMarkersImageFilter>::uuid;
}
std::string MorphologicalWatershedFromMarkersImageFilter::humanName() const
{
  return "Morphological Watershed From Markers Image Filter";
}
std::vector<std::string> MorphologicalWatershedFromMarkersImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "MorphologicalWatershedFromMarkers", "Watersheds", "Segmentation"};
}
Parameters MorphologicalWatershedFromMarkersImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_MarkWatershedLine_Key, "Mark Watershed Line",
                                                "Set/Get whether the watershed pixel must be marked or not. Default is true. Set it to false to not only avoid writing watershed pixels, it also "
                                                "decreases the algorithm complexity.",
                                                true));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The grayscale image data that will be flooded by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MarkerImageDataPath_Key, "Marker Cell Data",
                                                          "The marker/seed label image; must be an integer scalar array with the same number of tuples as the input.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_OutputImageArrayName_Key, "Output Cell Data", "The resulting label image (same element type as the marker image) will be stored in this Data Array inside the same group as the input data.",
      "Output Image Data"));
  return params;
}
IFilter::VersionType MorphologicalWatershedFromMarkersImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer MorphologicalWatershedFromMarkersImageFilter::clone() const
{
  return std::make_unique<MorphologicalWatershedFromMarkersImageFilter>();
}
IFilter::PreflightResult MorphologicalWatershedFromMarkersImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto markerArrayPath = filterArgs.value<DataPath>(k_MarkerImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);

  // Validate the marker/seed array: it must be an integer scalar type (the flood dispatches its label copy on an
  // integer element type) with the same number of tuples as the grayscale input (they share the image grid).
  const auto& markerArray = dataStructure.getDataRefAs<IDataArray>(markerArrayPath);
  const DataType markerType = markerArray.getDataType();
  if(nx::core::ImageProcessing::GetIntegerScalarTypes().count(markerType) == 0)
  {
    return {MakeErrorResult<OutputActions>(-8600, fmt::format("Marker array '{}' must be an integer scalar type, but is '{}'.", markerArrayPath.toString(), DataTypeToString(markerType)))};
  }
  if(markerArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(
        -8601, fmt::format("Marker array '{}' must be single-component (scalar), but has {} components.", markerArrayPath.toString(), markerArray.getNumberOfComponents()))};
  }
  if(markerArray.getNumberOfTuples() != inputArray.getNumberOfTuples())
  {
    return {MakeErrorResult<OutputActions>(-8602,
                                           fmt::format("Marker array tuple count ({}) does not match input array tuple count ({}).", markerArray.getNumberOfTuples(), inputArray.getNumberOfTuples()))};
  }

  // Validate the grayscale input (allowed type, single-component, tuple==geom cells) via the shared preflight, but
  // DISCARD its CreateArrayAction: that action would allocate the output of the INPUT (grayscale) type, whereas this
  // filter's output must be the MARKER's type. requireScalar=true because the flood is defined on scalar images.
  Result<OutputActions> grayscaleValidation =
      ImageProcessing::PreflightImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  if(grayscaleValidation.invalid())
  {
    return {std::move(grayscaleValidation)};
  }

  // Resident watershed retains the exact RAM flood and its available-memory guard. OOC watershed uses bounded
  // fixed-record voxel/queue storage and therefore requires the temporary-record capability. getPlannedStoreType()
  // reflects an upstream array's eventual backing while it is still an EmptyDataStore placeholder.
  const bool inputIsOutOfCore = inputArray.getIDataStoreRef().getPlannedStoreType() == IDataStore::StoreType::OutOfCore;
  Result<OutputActions> memoryCheck;
  if(inputIsOutOfCore)
  {
    if(!DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
    {
      return {MakeErrorResult<OutputActions>(
          -79047, fmt::format("Morphological watershed from markers input '{}' is planned out-of-core, but no temporary-record storage provider is registered for the external flood.",
                              selectedInputArray.toString()))};
    }
  }
  else
  {
    memoryCheck = ImageProcessing::ValidateWatershedFitsInMemory(inputArray.getDataType(), inputArray.getNumberOfTuples(), /*inputIsOutOfCore=*/false, /*errorCode=*/-79041,
                                                                 /*warningCode=*/-79043);
  }
  if(memoryCheck.invalid())
  {
    return {std::move(memoryCheck)};
  }

  // Create the output of the MARKER's type, inheriting the grayscale input's tuple/component shape + data format
  // (marker and grayscale share the grid + tuple count, so the input's shapes are correct; the format keeps an OOC
  // input's output OOC-backed).
  OutputActions outputActions;
  outputActions.appendAction(std::make_unique<CreateArrayAction>(markerType, inputArray.getTupleShape(), inputArray.getComponentShape(), outputArrayPath, inputArray.getDataFormat(), "", std::nullopt,
                                                                 DataStoreInitializationMode::DeferredZeroFill));
  // Preserve any future resident memory-check warnings on the Result<OutputActions> wrapper.
  Result<OutputActions> result{std::move(outputActions)};
  for(auto& warning : memoryCheck.warnings())
  {
    result.warnings().push_back(std::move(warning));
  }
  return {std::move(result)};
}
Result<> MorphologicalWatershedFromMarkersImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto markerArrayPath = filterArgs.value<DataPath>(k_MarkerImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  return ImageProcessing::ExecuteMorphologicalWatershedFromMarkersImageFilter(dataStructure, imageGeomPath, selectedInputArray, markerArrayPath, outputArrayPath, markWatershedLine, fullyConnected,
                                                                              shouldCancel, messageHandler);
}
} // namespace nx::core
