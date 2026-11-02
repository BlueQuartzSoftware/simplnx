#include "BinaryDilateImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
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
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <utility>

using namespace nx::core;

namespace
{
// Error/warning codes for this filter (verified free by grepping the codebase for -84xx codes; -8460..-8463 do
// not collide with any existing filter, unlike the -8400 block a naive copy from the grayscale filters would use).
constexpr int32 k_NonScalarInput = -8460;
constexpr int32 k_NonFiniteBinaryValue = -8461;
constexpr int32 k_BinaryValueOutOfRange = -8462;
constexpr int32 k_BinaryValueTruncated = -8463;
} // namespace

namespace nx::core
{
std::string BinaryDilateImageFilter::name() const
{
  return FilterTraits<BinaryDilateImageFilter>::name;
}
std::string BinaryDilateImageFilter::className() const
{
  return FilterTraits<BinaryDilateImageFilter>::className;
}
Uuid BinaryDilateImageFilter::uuid() const
{
  return FilterTraits<BinaryDilateImageFilter>::uuid;
}
std::string BinaryDilateImageFilter::humanName() const
{
  return "Binary Dilate Image Filter";
}
std::vector<std::string> BinaryDilateImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "BinaryDilate", "Binary", "Morphology", "MathematicalMorphology"};
}
Parameters BinaryDilateImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                        std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.",
                                                   static_cast<uint64>(ImageProcessing::KernelType::Ball), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The value written to voxels that are not foreground in the output. For an integer input image the value "
                                                   "must be finite and within the image type's range.",
                                                   0.0));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "Foreground Value",
                                                   "The value identifying the objects to dilate. Only voxels equal to this value are foreground; all other values are background. For an integer "
                                                   "input image the value must be finite and within the image type's range.",
                                                   1.0));
  params.insert(
      std::make_unique<BoolParameter>(k_BoundaryToForeground_Key, "Boundary To Foreground", "Whether voxels outside the image boundary are treated as foreground during the dilation.", false));

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
IFilter::VersionType BinaryDilateImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer BinaryDilateImageFilter::clone() const
{
  return std::make_unique<BinaryDilateImageFilter>();
}
IFilter::PreflightResult BinaryDilateImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
    return {MakeErrorResult<OutputActions>(k_NonScalarInput, fmt::format("Binary Dilate requires a single-component (scalar) input array, but '{}' has {} components.", selectedInputArray.toString(),
                                                                         inputArray.getNumberOfComponents()))};
  }

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // The Float64 foreground/background parameters are cast to the (integer) input element type by the binary
  // morphology façade. That conversion is UNDEFINED for a non-finite or out-of-range value, so validate the
  // parameters here (the array is guaranteed present/scalar/integer by the successful checks above).
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
Result<> BinaryDilateImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_KernelRadius_Key);
  auto kernelType = filterArgs.value<ChoicesParameter::ValueType>(k_KernelType_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto boundaryToForeground = filterArgs.value<bool>(k_BoundaryToForeground_Key);

  const ImageProcessing::StructuringElement se = ImageProcessing::MakeStructuringElement(
      static_cast<ImageProcessing::KernelType>(kernelType), {static_cast<int32>(kernelRadius[0]), static_cast<int32>(kernelRadius[1]), static_cast<int32>(kernelRadius[2])});

  return ImageProcessing::ExecuteBinaryMorphologyImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, se, ImageProcessing::MorphOp::Dilate, foregroundValue, backgroundValue,
                                                             boundaryToForeground, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_KernelTypeKey = "KernelType";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_ForegroundValueKey = "ForegroundValue";
constexpr StringLiteral k_BoundaryToForegroundKey = "BoundaryToForeground";
constexpr StringLiteral k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> BinaryDilateImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BinaryDilateImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_ForegroundValueKey, k_ForegroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_BoundaryToForegroundKey, k_BoundaryToForeground_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
