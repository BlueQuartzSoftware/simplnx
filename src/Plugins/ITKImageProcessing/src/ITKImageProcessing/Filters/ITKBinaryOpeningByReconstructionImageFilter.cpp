#include "ITKBinaryOpeningByReconstructionImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkBinaryOpeningByReconstructionImageFilter.h>

using namespace nx::core;

namespace cxITKBinaryOpeningByReconstructionImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;

struct ITKBinaryOpeningByReconstructionImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  float64 foregroundValue = 1.0;
  float64 backgroundValue = 0.0;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::BinaryOpeningByReconstructionImageFilter<InputImageT, itk::FlatStructuringElement<InputImageT::ImageDimension>>;
    auto filter = FilterType::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetForegroundValue(foregroundValue);
    filter->SetBackgroundValue(backgroundValue);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKBinaryOpeningByReconstructionImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKBinaryOpeningByReconstructionImageFilter::name() const
{
  return FilterTraits<ITKBinaryOpeningByReconstructionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryOpeningByReconstructionImageFilter::className() const
{
  return FilterTraits<ITKBinaryOpeningByReconstructionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryOpeningByReconstructionImageFilter::uuid() const
{
  return FilterTraits<ITKBinaryOpeningByReconstructionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryOpeningByReconstructionImageFilter::humanName() const
{
  return "ITK Binary Opening By Reconstruction Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryOpeningByReconstructionImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBinaryOpeningByReconstructionImage", "ITKBinaryMathematicalMorphology", "BinaryMathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryOpeningByReconstructionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                          std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.", static_cast<uint64>(itk::simple::sitkBall),
                                                   ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "Foreground Value", "Set the value in the image to consider as 'foreground'. Defaults to maximum value of PixelType.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value", "Set the value in eroded part of the image. Defaults to zero", 0.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryOpeningByReconstructionImageFilter::clone() const
{
  return std::make_unique<ITKBinaryOpeningByReconstructionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryOpeningByReconstructionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinaryOpeningByReconstructionImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryOpeningByReconstructionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKBinaryOpeningByReconstructionImageFilter::ITKBinaryOpeningByReconstructionImageFunctor itkFunctor = {kernelRadius, kernelType, foregroundValue, backgroundValue, fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKBinaryOpeningByReconstructionImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_KernelTypeKey = "KernelType";
constexpr StringLiteral k_ForegroundValueKey = "ForegroundValue";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKBinaryOpeningByReconstructionImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKBinaryOpeningByReconstructionImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_ForegroundValueKey, k_ForegroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
