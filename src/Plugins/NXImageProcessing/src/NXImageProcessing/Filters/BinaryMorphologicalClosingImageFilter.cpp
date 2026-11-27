#include "BinaryMorphologicalClosingImageFilter.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

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
#include "simplnx/Utilities/ImageProcessing/MorphologyLayout.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <utility>

using namespace nx::core;

namespace
{
// Error/warning codes for this filter (verified free by grepping the codebase for -84xx codes; -8490..-8493 do
// not collide with any existing filter -- the grayscale composites use -8390/-8400/-8410/-8420, Binary
// Erode/Dilate use -8440..-8443 / -8460..-8463, Binary Opening uses -8480..-8483).
constexpr int32 k_NonScalarInput = -8490;
constexpr int32 k_NonFiniteBinaryValue = -8491;
constexpr int32 k_BinaryValueOutOfRange = -8492;
constexpr int32 k_BinaryValueTruncated = -8493;
} // namespace

namespace nx::core
{
std::string BinaryMorphologicalClosingImageFilter::name() const
{
  return FilterTraits<BinaryMorphologicalClosingImageFilter>::name;
}
std::string BinaryMorphologicalClosingImageFilter::className() const
{
  return FilterTraits<BinaryMorphologicalClosingImageFilter>::className;
}
Uuid BinaryMorphologicalClosingImageFilter::uuid() const
{
  return FilterTraits<BinaryMorphologicalClosingImageFilter>::uuid;
}
std::string BinaryMorphologicalClosingImageFilter::humanName() const
{
  return "Binary Morphological Closing Image Filter";
}
std::vector<std::string> BinaryMorphologicalClosingImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "BinaryMorphologicalClosing", "Binary", "Morphology", "MathematicalMorphology"};
}
Parameters BinaryMorphologicalClosingImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt32ParameterType>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                            std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.",
                                                   static_cast<uint64>(ImageProcessing::KernelType::Ball), ChoicesParameter::ChoicesType{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64ParameterType>(k_ForegroundValue_Key, "Foreground Value",
                                                       "The value identifying the objects to close. Only voxels equal to this value are foreground; all other values are background. For an integer "
                                                       "input image the value must be finite and within the image type's range.",
                                                       1.0));
  params.insert(std::make_unique<BoolParameter>(k_SafeBorder_Key, "Safe Border", "A safe border is added to input image to avoid borders effects and remove it once the closing is done", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType BinaryMorphologicalClosingImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointerType BinaryMorphologicalClosingImageFilter::clone() const
{
  return std::make_unique<BinaryMorphologicalClosingImageFilter>();
}
IFilter::PreflightResult BinaryMorphologicalClosingImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                              const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto kernelRadius = filterArgs.value<VectorUInt32ParameterType::ValueType>(k_KernelRadius_Key);
  if(auto radiusResult = ImageProcessing::ValidateKernelRadius(kernelRadius); radiusResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(radiusResult), OutputActions{})};
  }
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& layoutInputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(auto layoutResult = ImageProcessing::ValidateMorphologyLayout(imageGeom.getDimensions(), kernelRadius, filterArgs.value<bool>(k_SafeBorder_Key), GetDataTypeSize(layoutInputArray.getDataType()));
     layoutResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(ImageProcessing::AddMorphologyLayoutGeometryContext(std::move(layoutResult), imageGeomPath), OutputActions{})};
  }

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(k_NonScalarInput, fmt::format("Binary Morphological Closing requires a single-component (scalar) input array, but '{}' has {} components.",
                                                                                           selectedInputArray.toString(), inputArray.getNumberOfComponents()))};
  }

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  if(resultOutputActions.invalid())
  {
    return {.output_actions = std::move(resultOutputActions)};
  }

  // The Float64 Foreground Value is cast to the (integer) input element type by the binary morphology façade;
  // that conversion is UNDEFINED for a non-finite or out-of-range value, so validate it here. Closing has no
  // user Background Value -- its internal background is 0 (or the type max when the foreground is 0), always
  // representable -- so the guard's background slot is passed a representative 0.0 (never out of range).
  const DataType inputType = inputArray.getDataType();
  Result<> paramGuard = ImageProcessing::ValidateBinaryFgBgInRange(inputType, foregroundValue, /*background=*/0.0, k_NonFiniteBinaryValue, k_BinaryValueOutOfRange, k_BinaryValueTruncated);
  if(paramGuard.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(paramGuard), OutputActions{})};
  }
  for(Warning& warning : paramGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  return {.output_actions = std::move(resultOutputActions)};
}
Result<> BinaryMorphologicalClosingImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorUInt32ParameterType::ValueType>(k_KernelRadius_Key);
  auto kernelType = filterArgs.value<ChoicesParameter::ValueType>(k_KernelType_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto safeBorder = filterArgs.value<bool>(k_SafeBorder_Key);

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(auto layoutResult = ImageProcessing::ValidateMorphologyLayout(imageGeom.getDimensions(), kernelRadius, filterArgs.value<bool>(k_SafeBorder_Key), GetDataTypeSize(inputArray.getDataType()));
     layoutResult.invalid())
  {
    return ImageProcessing::AddMorphologyLayoutGeometryContext(std::move(layoutResult), imageGeomPath);
  }
  auto kernelResult = ImageProcessing::MakeCheckedStructuringElement(static_cast<ImageProcessing::KernelType>(kernelType), kernelRadius);
  if(kernelResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(kernelResult));
  }
  const ImageProcessing::StructuringElement& se = kernelResult.value();

  // Closing derives its own internal background, so the façade's background argument is unused for Closing; pass 0.0.
  return ImageProcessing::ExecuteBinaryMorphologyCompositeImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, se, ImageProcessing::BinaryCompositeOp::Closing,
                                                                      foregroundValue, /*background=*/0.0, safeBorder, shouldCancel, messageHandler);
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_KernelTypeKey = "KernelType";
constexpr StringLiteralType k_ForegroundValueKey = "ForegroundValue";
constexpr StringLiteralType k_SafeBorderKey = "SafeBorder";
constexpr StringLiteralType k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteralType k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteralType k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace simpl
} // namespace

Result<Arguments> BinaryMorphologicalClosingImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BinaryMorphologicalClosingImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DoubleFilterParameterConverterType>(args, json, simpl::k_ForegroundValueKey, k_ForegroundValue_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_SafeBorderKey, k_SafeBorder_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::UInt32Vec3FilterParameterConverterType>(args, json, simpl::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
