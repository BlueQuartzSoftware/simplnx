#include "ITKErodeObjectMorphologyImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"


#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkErodeObjectMorphologyImageFilter.h>

using namespace complex;

namespace cxITKErodeObjectMorphologyImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKErodeObjectMorphologyImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  float64 objectValue = 1;
  float64 backgroundValue = 0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ErodeObjectMorphologyImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement< InputImageT::ImageDimension > >;
    auto filter = FilterType::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetObjectValue(objectValue);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace cxITKErodeObjectMorphologyImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKErodeObjectMorphologyImage::name() const
{
  return FilterTraits<ITKErodeObjectMorphologyImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKErodeObjectMorphologyImage::className() const
{
  return FilterTraits<ITKErodeObjectMorphologyImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKErodeObjectMorphologyImage::uuid() const
{
  return FilterTraits<ITKErodeObjectMorphologyImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKErodeObjectMorphologyImage::humanName() const
{
  return "ITK Erode Object Morphology Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKErodeObjectMorphologyImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKErodeObjectMorphologyImage", "ITKBinaryMathematicalMorphology", "BinaryMathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKErodeObjectMorphologyImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "KernelRadius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1), std::vector<std::string>{"X","Y","Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "KernelType", "", static_cast<uint64>(itk::simple::sitkBall), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64Parameter>(k_ObjectValue_Key, "ObjectValue", "", 1));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "Set the value to be assigned to eroded pixels", 0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKErodeObjectMorphologyImage::clone() const
{
  return std::make_unique<ITKErodeObjectMorphologyImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKErodeObjectMorphologyImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto objectValue = filterArgs.value<float64>(k_ObjectValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKErodeObjectMorphologyImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKErodeObjectMorphologyImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto objectValue = filterArgs.value<float64>(k_ObjectValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  const cxITKErodeObjectMorphologyImage::ITKErodeObjectMorphologyImageFunctor itkFunctor = {kernelRadius, kernelType, objectValue, backgroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
  
  return ITK::Execute<cxITKErodeObjectMorphologyImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
