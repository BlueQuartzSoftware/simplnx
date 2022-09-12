#include "ITKLaplacianRecursiveGaussianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkLaplacianRecursiveGaussianImageFilter.h>

using namespace complex;

namespace cxITKLaplacianRecursiveGaussianImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = float32;

struct ITKLaplacianRecursiveGaussianImageFunctor
{
  float64 sigma = 1.0;
  bool normalizeAcrossScale = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::LaplacianRecursiveGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetSigma(sigma);
    filter->SetNormalizeAcrossScale(normalizeAcrossScale);
    return filter;
  }
};
} // namespace cxITKLaplacianRecursiveGaussianImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImage::name() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImage::className() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKLaplacianRecursiveGaussianImage::uuid() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImage::humanName() const
{
  return "ITK Laplacian Recursive Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKLaplacianRecursiveGaussianImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKLaplacianRecursiveGaussianImage", "ITKImageFeature", "ImageFeature"};
}

//------------------------------------------------------------------------------
Parameters ITKLaplacianRecursiveGaussianImage::parameters() const
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
IFilter::UniquePointer ITKLaplacianRecursiveGaussianImage::clone() const
{
  return std::make_unique<ITKLaplacianRecursiveGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKLaplacianRecursiveGaussianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKLaplacianRecursiveGaussianImage::ArrayOptionsT, cxITKLaplacianRecursiveGaussianImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKLaplacianRecursiveGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  cxITKLaplacianRecursiveGaussianImage::ITKLaplacianRecursiveGaussianImageFunctor itkFunctor = {sigma, normalizeAcrossScale};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKLaplacianRecursiveGaussianImage::ArrayOptionsT, cxITKLaplacianRecursiveGaussianImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                                itkFunctor);
}
} // namespace complex
