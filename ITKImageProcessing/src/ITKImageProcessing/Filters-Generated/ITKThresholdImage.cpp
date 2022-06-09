#include "ITKThresholdImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkThresholdImageFilter.h>

using namespace complex;

namespace cxITKThresholdImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKThresholdImageFunctor
{
  float64 lower = 0.0;
  float64 upper = 1.0;
  float64 outsideValue = 0.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ThresholdImageFilter<InputImageT>;
    auto filter = FilterT::New();
    filter->SetLower(lower);
    filter->SetUpper(upper);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKThresholdImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKThresholdImage::name() const
{
  return FilterTraits<ITKThresholdImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKThresholdImage::className() const
{
  return FilterTraits<ITKThresholdImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKThresholdImage::uuid() const
{
  return FilterTraits<ITKThresholdImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKThresholdImage::humanName() const
{
  return "ITK Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKThresholdImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKThresholdImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKThresholdImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Lower_Key, "Lower", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Upper_Key, "Upper", "", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "", 0.0));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKThresholdImage::clone() const
{
  return std::make_unique<ITKThresholdImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKThresholdImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto lower = filterArgs.value<float64>(k_Lower_Key);
  auto upper = filterArgs.value<float64>(k_Upper_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKThresholdImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKThresholdImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto lower = filterArgs.value<float64>(k_Lower_Key);
  auto upper = filterArgs.value<float64>(k_Upper_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  cxITKThresholdImage::ITKThresholdImageFunctor itkFunctor = {lower, upper, outsideValue};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKThresholdImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
