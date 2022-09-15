#include "ITKGradientMagnitudeRecursiveGaussianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>

using namespace complex;

namespace cxITKGradientMagnitudeRecursiveGaussianImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = float32;

struct ITKGradientMagnitudeRecursiveGaussianImageFunctor
{
  float64 sigma = 1.0;
  bool normalizeAcrossScale = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::GradientMagnitudeRecursiveGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetSigma(sigma);
    filter->SetNormalizeAcrossScale(normalizeAcrossScale);
    return filter;
  }
};
} // namespace cxITKGradientMagnitudeRecursiveGaussianImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeRecursiveGaussianImage::name() const
{
  return FilterTraits<ITKGradientMagnitudeRecursiveGaussianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeRecursiveGaussianImage::className() const
{
  return FilterTraits<ITKGradientMagnitudeRecursiveGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGradientMagnitudeRecursiveGaussianImage::uuid() const
{
  return FilterTraits<ITKGradientMagnitudeRecursiveGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeRecursiveGaussianImage::humanName() const
{
  return "ITK Gradient Magnitude Recursive Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGradientMagnitudeRecursiveGaussianImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKGradientMagnitudeRecursiveGaussianImage", "ITKImageGradient", "ImageGradient"};
}

//------------------------------------------------------------------------------
Parameters ITKGradientMagnitudeRecursiveGaussianImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Sigma_Key, "Sigma", "", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "NormalizeAcrossScale", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKGradientMagnitudeRecursiveGaussianImage::clone() const
{
  return std::make_unique<ITKGradientMagnitudeRecursiveGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGradientMagnitudeRecursiveGaussianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKGradientMagnitudeRecursiveGaussianImage::ArrayOptionsT, cxITKGradientMagnitudeRecursiveGaussianImage::FilterOutputT>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGradientMagnitudeRecursiveGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  cxITKGradientMagnitudeRecursiveGaussianImage::ITKGradientMagnitudeRecursiveGaussianImageFunctor itkFunctor = {sigma, normalizeAcrossScale};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKGradientMagnitudeRecursiveGaussianImage::ArrayOptionsT, cxITKGradientMagnitudeRecursiveGaussianImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                                outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
