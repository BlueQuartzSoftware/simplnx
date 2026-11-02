#include "ErodeObjectMorphologyImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ObjectMorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
// Error/warning codes for this filter. Verified free by grepping the whole src/Plugins/ImageProcessing tree
// (and all of src/) for -85xx codes: the -8520 block is used by Dilate Object Morphology, so this filter uses
// the still-unused -8530 block.
constexpr int32 k_NonScalarInput = -8530;
constexpr int32 k_NonFiniteValue = -8531;
constexpr int32 k_ValueOutOfRange = -8532;
constexpr int32 k_ValueTruncated = -8533;
} // namespace

namespace nx::core
{
std::string ErodeObjectMorphologyImageFilter::name() const
{
  return FilterTraits<ErodeObjectMorphologyImageFilter>::name;
}
std::string ErodeObjectMorphologyImageFilter::className() const
{
  return FilterTraits<ErodeObjectMorphologyImageFilter>::className;
}
Uuid ErodeObjectMorphologyImageFilter::uuid() const
{
  return FilterTraits<ErodeObjectMorphologyImageFilter>::uuid;
}
std::string ErodeObjectMorphologyImageFilter::humanName() const
{
  return "Erode Object Morphology Image Filter";
}
std::vector<std::string> ErodeObjectMorphologyImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "ErodeObjectMorphology", "ObjectMorphology", "Morphology", "BinaryMathematicalMorphology"};
}
Parameters ErodeObjectMorphologyImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                          std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.",
                                                   static_cast<uint64>(ImageProcessing::KernelType::Ball), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64Parameter>(k_ObjectValue_Key, "Object Value",
                                                   "The pixel value of the 'Object' to be eroded. Voxels equal to this value are the object; every other value is background. For an integer input "
                                                   "image the value must be finite and within the image type's range.",
                                                   1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The value assigned to eroded pixels (painted over the border of the object). For an integer input image the value must be finite and within the "
                                                   "image type's range.",
                                                   0.0));

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
IFilter::VersionType ErodeObjectMorphologyImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer ErodeObjectMorphologyImageFilter::clone() const
{
  return std::make_unique<ErodeObjectMorphologyImageFilter>();
}
IFilter::PreflightResult ErodeObjectMorphologyImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto objectValue = filterArgs.value<float64>(k_ObjectValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(k_NonScalarInput, fmt::format("Erode Object Morphology requires a single-component (scalar) input array, but '{}' has {} components.",
                                                                         selectedInputArray.toString(), inputArray.getNumberOfComponents()))};
  }

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // Both the Float64 Object Value and Background Value are cast to the input element type by the
  // object-morphology façade. That conversion is UNDEFINED for a non-finite or out-of-range value on an
  // integer type (and non-finite would break the exact-equality object test on any type). Validate the Object
  // Value first, then the Background Value; return early on the FIRST error and accumulate any truncation
  // warnings from both (the array is guaranteed present/scalar by the checks above).
  const DataType inputType = inputArray.getDataType();
  Result<> objectGuard = ImageProcessing::ValidateScalarValueForType(inputType, objectValue, "Object Value", k_NonFiniteValue, k_ValueOutOfRange, k_ValueTruncated);
  if(objectGuard.invalid())
  {
    // Intentional: on an error we abort preflight and drop any warning accumulated so far (mirrors
    // ValidateBinaryFgBgInRange); the warning reappears once the erroring value is fixed.
    return {ConvertResultTo<OutputActions>(std::move(objectGuard), OutputActions{})};
  }
  Result<> backgroundGuard = ImageProcessing::ValidateScalarValueForType(inputType, backgroundValue, "Background Value", k_NonFiniteValue, k_ValueOutOfRange, k_ValueTruncated);
  if(backgroundGuard.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(backgroundGuard), OutputActions{})};
  }
  for(Warning& warning : objectGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  for(Warning& warning : backgroundGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  return {std::move(resultOutputActions)};
}
Result<> ErodeObjectMorphologyImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = filterArgs.value<ChoicesParameter::ValueType>(k_KernelType_Key);
  auto objectValue = filterArgs.value<float64>(k_ObjectValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  const ImageProcessing::StructuringElement se = ImageProcessing::MakeStructuringElement(
      static_cast<ImageProcessing::KernelType>(kernelType), {static_cast<int32>(kernelRadius[0]), static_cast<int32>(kernelRadius[1]), static_cast<int32>(kernelRadius[2])});

  return ImageProcessing::ExecuteObjectMorphologyImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, se, ImageProcessing::ObjectMorphOp::Erode, objectValue, backgroundValue,
                                                             shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_KernelTypeKey = "KernelType";
constexpr StringLiteral k_ObjectValueKey = "ObjectValue";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ErodeObjectMorphologyImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeObjectMorphologyImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_ObjectValueKey, k_ObjectValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
