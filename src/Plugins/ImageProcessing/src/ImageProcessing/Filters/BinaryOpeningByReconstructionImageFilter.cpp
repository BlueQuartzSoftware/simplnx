#include "BinaryOpeningByReconstructionImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
// fg/bg range-guard error codes for this filter (the Float64 foreground/background are cast to the integer input
// element type at execute; the guard makes that narrowing well-defined). -8570..-8572 + -8579 verified free.
constexpr int32 k_NonFiniteBinaryValue = -8570;
constexpr int32 k_BinaryValueOutOfRange = -8571;
constexpr int32 k_BinaryValueTruncated = -8572;
constexpr int32 k_ForegroundEqualsBackground = -8579;
} // namespace

namespace nx::core
{
std::string BinaryOpeningByReconstructionImageFilter::name() const
{
  return FilterTraits<BinaryOpeningByReconstructionImageFilter>::name;
}
std::string BinaryOpeningByReconstructionImageFilter::className() const
{
  return FilterTraits<BinaryOpeningByReconstructionImageFilter>::className;
}
Uuid BinaryOpeningByReconstructionImageFilter::uuid() const
{
  return FilterTraits<BinaryOpeningByReconstructionImageFilter>::uuid;
}
std::string BinaryOpeningByReconstructionImageFilter::humanName() const
{
  return "Binary Opening By Reconstruction Image Filter";
}
std::vector<std::string> BinaryOpeningByReconstructionImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "BinaryOpeningByReconstruction", "MathematicalMorphology", "Morphology"};
}
Parameters BinaryOpeningByReconstructionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                        std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.",
                                                   static_cast<uint64>(ImageProcessing::KernelType::Ball), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "Foreground Value",
                                                   "The value identifying the objects to process. Only voxels equal to this value are foreground; all other values are background. For an integer "
                                                   "input image the value must be finite and within the type's range.",
                                                   1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The value written to voxels that are not part of a surviving foreground object in the output. For an integer input image the value must be "
                                                   "finite and within the type's range.",
                                                   0.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Whether the connected components of the reconstruction are defined strictly by face connectivity (Off) or by face+edge+vertex connectivity (On).",
                                                false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The binary image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType BinaryOpeningByReconstructionImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer BinaryOpeningByReconstructionImageFilter::clone() const
{
  return std::make_unique<BinaryOpeningByReconstructionImageFilter>();
}
IFilter::PreflightResult BinaryOpeningByReconstructionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // Foreground and background must be distinct: a binary image is defined by two different labels, and fg==bg makes
  // the strict-binary input scan, the erode boundary fill, and the fg/bg-ordering reconstruction-op selection all
  // degenerate.
  if(foregroundValue == backgroundValue)
  {
    return {MakeErrorResult<OutputActions>(k_ForegroundEqualsBackground,
                                           fmt::format("Binary Opening By Reconstruction requires distinct Foreground and Background values, but both were {}.", foregroundValue))};
  }

  // requireScalar=true: binary morphology + reconstruction are defined on scalar images. IntegerOnly restricts to
  // the integer element types the legacy binary filter accepts; the shared preflight also validates the geometry.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::SameAsInput>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // The Float64 foreground/background parameters are cast to the (integer) input element type by the façade. That
  // conversion is UNDEFINED for a non-finite or out-of-range value, so validate them here (the array is guaranteed
  // present/scalar/integer by the successful preflight above).
  const DataType inputType = dataStructure.getDataRefAs<IDataArray>(selectedInputArray).getDataType();
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
Result<> BinaryOpeningByReconstructionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_KernelRadius_Key);
  auto kernelType = filterArgs.value<ChoicesParameter::ValueType>(k_KernelType_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const ImageProcessing::StructuringElement se = ImageProcessing::MakeStructuringElement(
      static_cast<ImageProcessing::KernelType>(kernelType), {static_cast<int32>(kernelRadius[0]), static_cast<int32>(kernelRadius[1]), static_cast<int32>(kernelRadius[2])});

  // Binary opening by reconstruction == binary erode with the structuring element, then binary
  // reconstruction-by-dilation of the eroded image under the original input (retain surviving foreground objects).
  return ImageProcessing::ExecuteBinaryOpeningByReconstructionImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, se, foregroundValue, backgroundValue, fullyConnected,
                                                                          shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_KernelTypeKey = "KernelType";
constexpr StringLiteral k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteral k_ForegroundValueKey = "ForegroundValue";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> BinaryOpeningByReconstructionImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BinaryOpeningByReconstructionImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_ForegroundValueKey, k_ForegroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
