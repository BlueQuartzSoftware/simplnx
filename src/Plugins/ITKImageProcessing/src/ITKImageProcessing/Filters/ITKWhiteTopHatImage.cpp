#include "ITKWhiteTopHatImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"


#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkWhiteTopHatImageFilter.h>

using namespace complex;

namespace cxITKWhiteTopHatImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKWhiteTopHatImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  bool safeBorder = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::WhiteTopHatImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement< InputImageT::ImageDimension > >;
    auto filter = FilterType::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetSafeBorder(safeBorder);
    return filter;
  }
};
} // namespace cxITKWhiteTopHatImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKWhiteTopHatImage::name() const
{
  return FilterTraits<ITKWhiteTopHatImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKWhiteTopHatImage::className() const
{
  return FilterTraits<ITKWhiteTopHatImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKWhiteTopHatImage::uuid() const
{
  return FilterTraits<ITKWhiteTopHatImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKWhiteTopHatImage::humanName() const
{
  return "ITK White Top Hat Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKWhiteTopHatImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKWhiteTopHatImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKWhiteTopHatImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "KernelRadius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1), std::vector<std::string>{"X","Y","Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "KernelType", "", static_cast<uint64>(itk::simple::sitkBall), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<BoolParameter>(k_SafeBorder_Key, "SafeBorder", "A safe border is added to input image to avoid borders effects and remove it once the closing is done", true));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKWhiteTopHatImage::clone() const
{
  return std::make_unique<ITKWhiteTopHatImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKWhiteTopHatImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto safeBorder = filterArgs.value<bool>(k_SafeBorder_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKWhiteTopHatImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKWhiteTopHatImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto safeBorder = filterArgs.value<bool>(k_SafeBorder_Key);

  const cxITKWhiteTopHatImage::ITKWhiteTopHatImageFunctor itkFunctor = {kernelRadius, kernelType, safeBorder};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
  
  return ITK::Execute<cxITKWhiteTopHatImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
