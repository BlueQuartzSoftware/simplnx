#include "ITKBinaryMorphologicalClosingImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkBinaryMorphologicalClosingImageFilter.h>

using namespace complex;

namespace cxITKBinaryMorphologicalClosingImage
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;

struct ITKBinaryMorphologicalClosingImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  float64 foregroundValue = 1.0;
  bool safeBorder = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BinaryMorphologicalClosingImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement<InputImageT::ImageDimension>>;
    auto filter = FilterT::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetForegroundValue(foregroundValue);
    filter->SetSafeBorder(safeBorder);
    return filter;
  }
};
} // namespace cxITKBinaryMorphologicalClosingImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinaryMorphologicalClosingImage::name() const
{
  return FilterTraits<ITKBinaryMorphologicalClosingImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryMorphologicalClosingImage::className() const
{
  return FilterTraits<ITKBinaryMorphologicalClosingImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryMorphologicalClosingImage::uuid() const
{
  return FilterTraits<ITKBinaryMorphologicalClosingImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryMorphologicalClosingImage::humanName() const
{
  return "ITK Binary Morphological Closing Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryMorphologicalClosingImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKBinaryMorphologicalClosingImage", "ITKBinaryMathematicalMorphology", "BinaryMathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryMorphologicalClosingImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "KernelRadius", "", std::vector<uint32>(3, 1), std::vector<std::string>(3)));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "KernelType", "", static_cast<uint64>(itk::simple::sitkBall), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_SafeBorder_Key, "SafeBorder", "", true));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryMorphologicalClosingImage::clone() const
{
  return std::make_unique<ITKBinaryMorphologicalClosingImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryMorphologicalClosingImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto safeBorder = filterArgs.value<bool>(k_SafeBorder_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinaryMorphologicalClosingImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryMorphologicalClosingImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto safeBorder = filterArgs.value<bool>(k_SafeBorder_Key);

  cxITKBinaryMorphologicalClosingImage::ITKBinaryMorphologicalClosingImageFunctor itkFunctor = {kernelRadius, kernelType, foregroundValue, safeBorder};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKBinaryMorphologicalClosingImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
