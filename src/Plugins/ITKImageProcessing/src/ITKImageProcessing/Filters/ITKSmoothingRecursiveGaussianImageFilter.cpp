#include "ITKSmoothingRecursiveGaussianImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <itkSmoothingRecursiveGaussianImageFilter.h>

using namespace nx::core;

namespace cxITKSmoothingRecursiveGaussianImageFilter
{
// using ArrayOptionsType = ITK::UNKNOWN PIXEL TYPE;
using ArrayOptionsType = ITK::SignedScalarPixelIdTypeList;

struct ITKSmoothingRecursiveGaussianImageFilterFunctor
{
  using SigmaInputRadiusType = std::vector<float64>;
  SigmaInputRadiusType sigma = std::vector<double>(3, 1.0);
  bool normalizeAcrossScale = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::SmoothingRecursiveGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    // Set the Sigma Filter Property
    {
      using RadiusType = typename FilterType::RadiusType;
      auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);
      filter->SetRadius(convertedRadius);
    }

    filter->SetNormalizeAcrossScale(normalizeAcrossScale);
    return filter;
  }
};
} // namespace cxITKSmoothingRecursiveGaussianImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKSmoothingRecursiveGaussianImageFilter::name() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKSmoothingRecursiveGaussianImageFilter::className() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSmoothingRecursiveGaussianImageFilter::uuid() const
{
  return FilterTraits<ITKSmoothingRecursiveGaussianImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSmoothingRecursiveGaussianImageFilter::humanName() const
{
  return "ITK Smoothing Recursive Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSmoothingRecursiveGaussianImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKSmoothingRecursiveGaussianImageFilter", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKSmoothingRecursiveGaussianImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(
      k_NormalizeAcrossScale_Key, "Normalize Across Scale",
      "Set/Get the flag for normalizing the Gaussian over scale-space. This method does not effect the output of this filter.  \see RecursiveGaussianImageFilter::SetNormalizeAcrossScale", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSmoothingRecursiveGaussianImageFilter::clone() const
{
  return std::make_unique<ITKSmoothingRecursiveGaussianImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSmoothingRecursiveGaussianImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto sigma = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Sigma_Key);

  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSmoothingRecursiveGaussianImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSmoothingRecursiveGaussianImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto sigma = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Sigma_Key);

  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  const cxITKSmoothingRecursiveGaussianImageFilter::ITKSmoothingRecursiveGaussianImageFilterFunctor itkFunctor = {sigma, normalizeAcrossScale};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKSmoothingRecursiveGaussianImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
