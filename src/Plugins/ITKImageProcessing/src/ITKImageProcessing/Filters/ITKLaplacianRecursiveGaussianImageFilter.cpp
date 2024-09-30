#include "ITKLaplacianRecursiveGaussianImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkLaplacianRecursiveGaussianImageFilter.h>

using namespace nx::core;

namespace cxITKLaplacianRecursiveGaussianImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKLaplacianRecursiveGaussianImageFilterFunctor
{
  float64 sigma = 1.0;
  bool normalizeAcrossScale = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::LaplacianRecursiveGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetSigma(sigma);
    filter->SetNormalizeAcrossScale(normalizeAcrossScale);
    return filter;
  }
};
} // namespace cxITKLaplacianRecursiveGaussianImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImageFilter::name() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImageFilter::className() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKLaplacianRecursiveGaussianImageFilter::uuid() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImageFilter::humanName() const
{
  return "ITK Laplacian Recursive Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKLaplacianRecursiveGaussianImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKLaplacianRecursiveGaussianImageFilter", "ITKImageFeature", "ImageFeature"};
}

//------------------------------------------------------------------------------
Parameters ITKLaplacianRecursiveGaussianImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Sigma_Key, "Sigma", "Set/Get Sigma value. Sigma is measured in the units of image spacing.", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "Normalize Across Scale",
                                                "Define which normalization factor will be used for the Gaussian. See RecursiveGaussianImageFilter::SetNormalizeAcrossScale", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKLaplacianRecursiveGaussianImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKLaplacianRecursiveGaussianImageFilter::clone() const
{
  return std::make_unique<ITKLaplacianRecursiveGaussianImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKLaplacianRecursiveGaussianImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKLaplacianRecursiveGaussianImageFilter::ArrayOptionsType, cxITKLaplacianRecursiveGaussianImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKLaplacianRecursiveGaussianImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  const cxITKLaplacianRecursiveGaussianImageFilter::ITKLaplacianRecursiveGaussianImageFilterFunctor itkFunctor = {sigma, normalizeAcrossScale};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKLaplacianRecursiveGaussianImageFilter::ArrayOptionsType, cxITKLaplacianRecursiveGaussianImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                                  outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
