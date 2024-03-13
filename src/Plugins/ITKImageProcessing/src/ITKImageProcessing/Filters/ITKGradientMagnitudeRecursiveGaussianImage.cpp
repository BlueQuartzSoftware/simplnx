#include "ITKGradientMagnitudeRecursiveGaussianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>

using namespace nx::core;

namespace cxITKGradientMagnitudeRecursiveGaussianImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKGradientMagnitudeRecursiveGaussianImageFunctor
{
  float64 sigma = 1.0;
  bool normalizeAcrossScale = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::GradientMagnitudeRecursiveGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetSigma(sigma);
    filter->SetNormalizeAcrossScale(normalizeAcrossScale);
    return filter;
  }
};
} // namespace cxITKGradientMagnitudeRecursiveGaussianImage

namespace nx::core
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
  return {className(), "ITKImageProcessing", "ITKGradientMagnitudeRecursiveGaussianImage", "ITKImageGradient", "ImageGradient"};
}

//------------------------------------------------------------------------------
Parameters ITKGradientMagnitudeRecursiveGaussianImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Sigma_Key, "Sigma", "Set/Get Sigma value. Sigma is measured in the units of image spacing.", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "Normalize Across Scale",
                                                "Set/Get the normalization factor that will be used for the Gaussian. RecursiveGaussianImageFilter::SetNormalizeAcrossScale", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

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
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKGradientMagnitudeRecursiveGaussianImage::ArrayOptionsType, cxITKGradientMagnitudeRecursiveGaussianImage::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGradientMagnitudeRecursiveGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  const cxITKGradientMagnitudeRecursiveGaussianImage::ITKGradientMagnitudeRecursiveGaussianImageFunctor itkFunctor = {sigma, normalizeAcrossScale};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKGradientMagnitudeRecursiveGaussianImage::ArrayOptionsType, cxITKGradientMagnitudeRecursiveGaussianImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                                      outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
