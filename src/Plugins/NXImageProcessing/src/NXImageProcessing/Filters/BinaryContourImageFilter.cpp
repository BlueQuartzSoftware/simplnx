#include "BinaryContourImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ContourEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
// Error/warning codes for this filter. Verified free by grepping the whole src/Plugins/ImageProcessing tree for
// -84xx/-85xx codes: the -84xx blocks up to -8493 are already taken (grayscale composites -8390/-8400/-8410/
// -8420, Binary Erode -8440..-8443, Binary Dilate -8460..-8463, Binary Opening -8480..-8483, Binary Closing
// -8490..-8493), so this filter uses the still-unused -8500 block.
constexpr int32 k_NonScalarInput = -8500;
constexpr int32 k_NonFiniteBinaryValue = -8501;
constexpr int32 k_BinaryValueOutOfRange = -8502;
constexpr int32 k_BinaryValueTruncated = -8503;
} // namespace

namespace nx::core
{
std::string BinaryContourImageFilter::name() const
{
  return FilterTraits<BinaryContourImageFilter>::name;
}
std::string BinaryContourImageFilter::className() const
{
  return FilterTraits<BinaryContourImageFilter>::className;
}
Uuid BinaryContourImageFilter::uuid() const
{
  return FilterTraits<BinaryContourImageFilter>::uuid;
}
std::string BinaryContourImageFilter::humanName() const
{
  return "Binary Contour Image Filter";
}
std::vector<std::string> BinaryContourImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "BinaryContour", "Binary", "Contour", "ImageLabel"};
}
Parameters BinaryContourImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Whether the connected components are defined strictly by face connectivity (off) or by face+edge+vertex connectivity (on). Default is off. "
                                                "For objects that are 1 pixel wide, use Fully Connected on.",
                                                false));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "Foreground Value",
                                                   "The value identifying the objects in the input and output images. Only voxels equal to this value are objects; every other value is passed "
                                                   "through unchanged. For an integer input image the value must be finite and within the image type's range.",
                                                   1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The value written to voxels that are not on the border of the objects. For an integer input image the value must be finite and within the "
                                                   "image type's range.",
                                                   0.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType BinaryContourImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer BinaryContourImageFilter::clone() const
{
  return std::make_unique<BinaryContourImageFilter>();
}
IFilter::PreflightResult BinaryContourImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(k_NonScalarInput, fmt::format("Binary Contour requires a single-component (scalar) input array, but '{}' has {} components.", selectedInputArray.toString(),
                                                                         inputArray.getNumberOfComponents()))};
  }

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // The Float64 foreground/background parameters are cast to the (integer) input element type by the contour
  // façade. That conversion is UNDEFINED for a non-finite or out-of-range value, so validate the parameters
  // here (the array is guaranteed present/scalar/integer by the successful checks above).
  const DataType inputType = inputArray.getDataType();
  Result<> paramGuard = ImageProcessing::ValidateBinaryFgBgInRange(inputType, foregroundValue, backgroundValue, k_NonFiniteBinaryValue, k_BinaryValueOutOfRange, k_BinaryValueTruncated);
  if(paramGuard.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(paramGuard), OutputActions{})};
  }
  for(Warning& warning : paramGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  return {std::move(resultOutputActions)};
}
Result<> BinaryContourImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);

  // The factory casts fg/bg to the (integer) element type inside the type-dispatched façade; preflight has
  // already guaranteed the cast is well-defined.
  const ImageProcessing::BinaryContourPredicateFactory predicateFactory{foregroundValue, backgroundValue};

  return ImageProcessing::ExecuteContourImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, fullyConnected, predicateFactory, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_ForegroundValueKey = "ForegroundValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> BinaryContourImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BinaryContourImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_ForegroundValueKey, k_ForegroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
