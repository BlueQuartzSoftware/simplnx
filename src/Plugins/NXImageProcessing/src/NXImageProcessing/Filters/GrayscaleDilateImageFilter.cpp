#include "GrayscaleDilateImageFilter.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyLayout.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace nx::core
{
std::string GrayscaleDilateImageFilter::name() const
{
  return FilterTraits<GrayscaleDilateImageFilter>::name;
}
std::string GrayscaleDilateImageFilter::className() const
{
  return FilterTraits<GrayscaleDilateImageFilter>::className;
}
Uuid GrayscaleDilateImageFilter::uuid() const
{
  return FilterTraits<GrayscaleDilateImageFilter>::uuid;
}
std::string GrayscaleDilateImageFilter::humanName() const
{
  return "Grayscale Dilate Image Filter";
}
std::vector<std::string> GrayscaleDilateImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "GrayscaleDilate", "Morphology", "MathematicalMorphology"};
}
Parameters GrayscaleDilateImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt32ParameterType>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                            std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.",
                                                   static_cast<uint64>(ImageProcessing::KernelType::Ball), ChoicesParameter::ChoicesType{"Annulus", "Ball", "Box", "Cross"}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType GrayscaleDilateImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointerType GrayscaleDilateImageFilter::clone() const
{
  return std::make_unique<GrayscaleDilateImageFilter>();
}
IFilter::PreflightResult GrayscaleDilateImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                   const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto kernelRadius = filterArgs.value<VectorUInt32ParameterType::ValueType>(k_KernelRadius_Key);
  if(auto radiusResult = ImageProcessing::ValidateKernelRadius(kernelRadius); radiusResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(radiusResult), OutputActions{})};
  }
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& layoutInputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(auto layoutResult = ImageProcessing::ValidateMorphologyLayout(imageGeom.getDimensions(), kernelRadius, false, GetDataTypeSize(layoutInputArray.getDataType())); layoutResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(ImageProcessing::AddMorphologyLayoutGeometryContext(std::move(layoutResult), imageGeomPath), OutputActions{})};
  }

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-8370, fmt::format("Grayscale Dilate requires a single-component (scalar) input array, but '{}' has {} components.",
                                                                                selectedInputArray.toString(), inputArray.getNumberOfComponents()))};
  }
  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {.output_actions = std::move(resultOutputActions)};
}
Result<> GrayscaleDilateImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorUInt32ParameterType::ValueType>(k_KernelRadius_Key);
  auto kernelType = filterArgs.value<ChoicesParameter::ValueType>(k_KernelType_Key);

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(auto layoutResult = ImageProcessing::ValidateMorphologyLayout(imageGeom.getDimensions(), kernelRadius, false, GetDataTypeSize(inputArray.getDataType())); layoutResult.invalid())
  {
    return ImageProcessing::AddMorphologyLayoutGeometryContext(std::move(layoutResult), imageGeomPath);
  }
  auto kernelResult = ImageProcessing::MakeCheckedStructuringElement(static_cast<ImageProcessing::KernelType>(kernelType), kernelRadius);
  if(kernelResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(kernelResult));
  }
  const ImageProcessing::StructuringElement& se = kernelResult.value();

  return ImageProcessing::ExecuteMorphologyImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, se, ImageProcessing::MorphOp::Dilate, shouldCancel, messageHandler);
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_KernelTypeKey = "KernelType";
constexpr StringLiteralType k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteralType k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteralType k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace simpl
} // namespace

Result<Arguments> GrayscaleDilateImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = GrayscaleDilateImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::UInt32Vec3FilterParameterConverterType>(args, json, simpl::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
