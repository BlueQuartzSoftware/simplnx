#include "ITKMaskImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkMaskImageFilter.h>

using namespace complex;

namespace cxITKMaskImage
{
using ArrayOptionsType = ITK::ScalarVectorPixelIdTypeList;

struct ITKMaskImageFunctor
{
  float64 outsideValue = 0;
  float64 maskingValue = 0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MaskImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetOutsideValue(outsideValue);
    filter->SetMaskingValue(maskingValue);
    return filter;
  }
};
} // namespace cxITKMaskImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMaskImage::name() const
{
  return FilterTraits<ITKMaskImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMaskImage::className() const
{
  return FilterTraits<ITKMaskImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMaskImage::uuid() const
{
  return FilterTraits<ITKMaskImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMaskImage::humanName() const
{
  return "ITK Mask Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMaskImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMaskImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKMaskImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "Method to explicitly set the outside value of the mask. Defaults to 0", 0));
  params.insert(std::make_unique<Float64Parameter>(k_MaskingValue_Key, "MaskingValue", "Method to explicitly set the masking value of the mask. Defaults to 0", 0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetNonLabelPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMaskImage::clone() const
{
  return std::make_unique<ITKMaskImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMaskImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskingValue = filterArgs.value<float64>(k_MaskingValue_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMaskImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMaskImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto maskingValue = filterArgs.value<float64>(k_MaskingValue_Key);

  const cxITKMaskImage::ITKMaskImageFunctor itkFunctor = {outsideValue, maskingValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKMaskImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
