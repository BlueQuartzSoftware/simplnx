#include "ITKSmoothingRecursiveGaussianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkSmoothingRecursiveGaussianImageFilter.h>

using namespace complex;

namespace cxITKSmoothingRecursiveGaussianImage
{
using ArrayOptionsT = ITK::UNKNOWN PIXEL TYPE;

struct ITKSmoothingRecursiveGaussianImageFunctor
{
  using InputRadiusType = std::vector<float64>;
  InputRadiusType sigma = std::vector<double>(3, 1.0);
  bool normalizeAcrossScale = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::SmoothingRecursiveGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    // Set the Sigma Filter Property
    using RadiusType = typename FilterT::RadiusType;
    auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);
    filter->SetRadius(convertedRadius);
    filter->SetNormalizeAcrossScale(normalizeAcrossScale);
    return filter;
  }
};
} // namespace cxITKSmoothingRecursiveGaussianImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSmoothingRecursiveGaussianImage::name() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKSmoothingRecursiveGaussianImage::className() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSmoothingRecursiveGaussianImage::uuid() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSmoothingRecursiveGaussianImage::humanName() const
{
  return "ITK Smoothing Recursive Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSmoothingRecursiveGaussianImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKSmoothingRecursiveGaussianImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKSmoothingRecursiveGaussianImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_Radius_Key, "Radius", "Radius Dimensions XYZ", std::vector<double>(3, 1.0), std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "NormalizeAcrossScale", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, UNKNOWN INPUT PIXEL TYPE));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSmoothingRecursiveGaussianImage::clone() const
{
  return std::make_unique<ITKSmoothingRecursiveGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSmoothingRecursiveGaussianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto sigma = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSmoothingRecursiveGaussianImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSmoothingRecursiveGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto sigma = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  cxITKSmoothingRecursiveGaussianImage::ITKSmoothingRecursiveGaussianImageFunctor itkFunctor = {sigma, normalizeAcrossScale};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKSmoothingRecursiveGaussianImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
