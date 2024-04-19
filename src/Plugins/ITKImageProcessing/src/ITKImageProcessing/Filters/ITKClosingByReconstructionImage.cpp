#include "ITKClosingByReconstructionImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkClosingByReconstructionImageFilter.h>

using namespace nx::core;

namespace cxITKClosingByReconstructionImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKClosingByReconstructionImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  bool fullyConnected = false;
  bool preserveIntensities = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ClosingByReconstructionImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement<InputImageT::ImageDimension>>;
    auto filter = FilterType::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetFullyConnected(fullyConnected);
    filter->SetPreserveIntensities(preserveIntensities);
    return filter;
  }
};
} // namespace cxITKClosingByReconstructionImage

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKClosingByReconstructionImage::name() const
{
  return FilterTraits<ITKClosingByReconstructionImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKClosingByReconstructionImage::className() const
{
  return FilterTraits<ITKClosingByReconstructionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKClosingByReconstructionImage::uuid() const
{
  return FilterTraits<ITKClosingByReconstructionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKClosingByReconstructionImage::humanName() const
{
  return "ITK Closing By Reconstruction Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKClosingByReconstructionImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKClosingByReconstructionImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKClosingByReconstructionImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                          std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross", static_cast<uint64>(itk::simple::sitkBall),
                                                   ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected Components",
                                                "Whether the connected components are defined strictly by face connectivity (False) or by face+edge+vertex connectivity (True). Default is False"
                                                "For objects that are 1 pixel wide, use True.",
                                                false));
  params.insert(std::make_unique<BoolParameter>(
      k_PreserveIntensities_Key, "Preserve Intensities",
      "Set/Get whether the original intensities of the image retained for those pixels unaffected by the opening by reconstruction. If Off, the output pixel contrast will be reduced.", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Array Name",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKClosingByReconstructionImage::clone() const
{
  return std::make_unique<ITKClosingByReconstructionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKClosingByReconstructionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto preserveIntensities = filterArgs.value<bool>(k_PreserveIntensities_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKClosingByReconstructionImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKClosingByReconstructionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto preserveIntensities = filterArgs.value<bool>(k_PreserveIntensities_Key);

  const cxITKClosingByReconstructionImage::ITKClosingByReconstructionImageFunctor itkFunctor = {kernelRadius, kernelType, fullyConnected, preserveIntensities};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKClosingByReconstructionImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_KernelTypeKey = "KernelType";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_PreserveIntensitiesKey = "PreserveIntensities";
constexpr StringLiteral k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKClosingByReconstructionImage::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKClosingByReconstructionImage().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_PreserveIntensitiesKey, k_PreserveIntensities_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
