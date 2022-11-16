#include "ITKGradientMagnitudeImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkGradientMagnitudeImageFilter.h>

using namespace complex;

namespace cxITKGradientMagnitudeImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

template <class PixelT>
using FilterOutputT = float32;

struct ITKGradientMagnitudeImageFunctor
{
  bool useImageSpacing = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::GradientMagnitudeImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKGradientMagnitudeImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeImage::name() const
{
  return FilterTraits<ITKGradientMagnitudeImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeImage::className() const
{
  return FilterTraits<ITKGradientMagnitudeImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGradientMagnitudeImage::uuid() const
{
  return FilterTraits<ITKGradientMagnitudeImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeImage::humanName() const
{
  return "ITK Gradient Magnitude Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGradientMagnitudeImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKGradientMagnitudeImage", "ITKImageGradient", "ImageGradient"};
}

//------------------------------------------------------------------------------
Parameters ITKGradientMagnitudeImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "The image data that will be processed by this filter.", DataPath{}, complex::GetAllNumericTypes()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "The result of the processing will be stored in this Data Array.", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing", "Whether or not the filter will use the spacing of the input image in its calculations", true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKGradientMagnitudeImage::clone() const
{
  return std::make_unique<ITKGradientMagnitudeImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGradientMagnitudeImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKGradientMagnitudeImage::ArrayOptionsT, cxITKGradientMagnitudeImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGradientMagnitudeImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  cxITKGradientMagnitudeImage::ITKGradientMagnitudeImageFunctor itkFunctor = {useImageSpacing};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKGradientMagnitudeImage::ArrayOptionsT, cxITKGradientMagnitudeImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                              shouldCancel);
}
} // namespace complex
