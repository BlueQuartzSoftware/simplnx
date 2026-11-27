#include "BlackTopHatImageFilter.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
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

using namespace nx::core;

namespace nx::core
{
std::string BlackTopHatImageFilter::name() const
{
  return FilterTraits<BlackTopHatImageFilter>::name;
}
std::string BlackTopHatImageFilter::className() const
{
  return FilterTraits<BlackTopHatImageFilter>::className;
}
Uuid BlackTopHatImageFilter::uuid() const
{
  return FilterTraits<BlackTopHatImageFilter>::uuid;
}
std::string BlackTopHatImageFilter::humanName() const
{
  return "Black Top Hat Image Filter";
}
std::vector<std::string> BlackTopHatImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "BlackTopHat", "Morphology", "MathematicalMorphology"};
}
Parameters BlackTopHatImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt32ParameterType>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                            std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.",
                                                   static_cast<uint64>(ImageProcessing::KernelType::Ball), ChoicesParameter::ChoicesType{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<BoolParameter>(k_SafeBorder_Key, "Safe Border", "A safe border is added to input image to avoid borders effects and remove it once the closing is done", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          nx::core::ImageProcessing::GetScalarNumericTypes(), ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType BlackTopHatImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointerType BlackTopHatImageFilter::clone() const
{
  return std::make_unique<BlackTopHatImageFilter>();
}
IFilter::PreflightResult BlackTopHatImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
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
  if(auto layoutResult = ImageProcessing::ValidateMorphologyLayout(imageGeom.getDimensions(), kernelRadius, filterArgs.value<bool>(k_SafeBorder_Key), GetDataTypeSize(layoutInputArray.getDataType()));
     layoutResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(ImageProcessing::AddMorphologyLayoutGeometryContext(std::move(layoutResult), imageGeomPath), OutputActions{})};
  }

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> BlackTopHatImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorUInt32ParameterType::ValueType>(k_KernelRadius_Key);
  auto kernelType = filterArgs.value<ChoicesParameter::ValueType>(k_KernelType_Key);
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

  return ImageProcessing::ExecuteMorphologyCompositeImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, se, ImageProcessing::MorphCompositeOp::BlackTopHat, safeBorder,
                                                                shouldCancel, messageHandler);
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_KernelTypeKey = "KernelType";
constexpr StringLiteralType k_SafeBorderKey = "SafeBorder";
constexpr StringLiteralType k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteralType k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteralType k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace simpl
} // namespace

Result<Arguments> BlackTopHatImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BlackTopHatImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_SafeBorderKey, k_SafeBorder_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::UInt32Vec3FilterParameterConverterType>(args, json, simpl::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
