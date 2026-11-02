#include "MorphologicalWatershedImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string MorphologicalWatershedImageFilter::name() const
{
  return FilterTraits<MorphologicalWatershedImageFilter>::name;
}
std::string MorphologicalWatershedImageFilter::className() const
{
  return FilterTraits<MorphologicalWatershedImageFilter>::className;
}
Uuid MorphologicalWatershedImageFilter::uuid() const
{
  return FilterTraits<MorphologicalWatershedImageFilter>::uuid;
}
std::string MorphologicalWatershedImageFilter::humanName() const
{
  return "Morphological Watershed Image Filter";
}
std::vector<std::string> MorphologicalWatershedImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "MorphologicalWatershed", "Watersheds", "Segmentation"};
}

Parameters MorphologicalWatershedImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Level_Key, "Level", "Set the 'level' variable to the filter", 0.0));
  params.insert(std::make_unique<BoolParameter>(
      k_MarkWatershedLine_Key, "MarkWatershedLine",
      "Set/Get whether the watershed pixel must be marked or not. Default is true. Set it to false do not only avoid writing watershed pixels, it also decrease algorithm complexity.", true));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));

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

IFilter::VersionType MorphologicalWatershedImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer MorphologicalWatershedImageFilter::clone() const
{
  return std::make_unique<MorphologicalWatershedImageFilter>();
}

IFilter::PreflightResult MorphologicalWatershedImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: watershed segmentation is defined on scalar images. The output is a FIXED uint32 label image
  // (matching the legacy ITK FilterOutputType), so AlwaysUInt32 maps every input type to a uint32 output array.
  // AllNumeric admits every scalar type (the legacy ScalarPixelIdTypeList, incl. float32/float64). There is no
  // minimum-dimension requirement (no dim guard follows).
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysUInt32>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // Resident watershed retains the exact RAM flood and its available-memory guard. OOC watershed uses bounded
  // fixed-record voxel/queue storage and therefore requires the temporary-record capability.
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  // getPlannedStoreType() (not getStoreType()) reflects the input's eventual InMemory/OutOfCore backing even when it is
  // still an EmptyDataStore placeholder at preflight (getStoreType() would report Empty for a pipeline-upstream array).
  const bool inputIsOutOfCore = inputArray.getIDataStoreRef().getPlannedStoreType() == IDataStore::StoreType::OutOfCore;
  Result<OutputActions> memoryCheck;
  if(inputIsOutOfCore)
  {
    if(!DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
    {
      return {MakeErrorResult<OutputActions>(
          -79048,
          fmt::format("Morphological watershed input '{}' is planned out-of-core, but no temporary-record storage provider is registered for the external flood.", selectedInputArray.toString()))};
    }
  }
  else
  {
    memoryCheck = ImageProcessing::ValidateWatershedFitsInMemory(inputArray.getDataType(), inputArray.getNumberOfTuples(), /*inputIsOutOfCore=*/false, /*errorCode=*/-79042,
                                                                 /*warningCode=*/-79044);
  }
  if(memoryCheck.invalid())
  {
    return {std::move(memoryCheck)};
  }
  for(auto& warning : memoryCheck.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }

  return {std::move(resultOutputActions)};
}

Result<> MorphologicalWatershedImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto level = filterArgs.value<float64>(k_Level_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  return ImageProcessing::ExecuteMorphologicalWatershedImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, level, markWatershedLine, fullyConnected, shouldCancel,
                                                                   messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_LevelKey = "Level";
constexpr StringLiteral k_MarkWatershedLineKey = "MarkWatershedLine";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> MorphologicalWatershedImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MorphologicalWatershedImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_LevelKey, k_Level_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_MarkWatershedLineKey, k_MarkWatershedLine_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
