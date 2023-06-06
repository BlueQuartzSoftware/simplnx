#include "ITKDiscreteGaussianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkDiscreteGaussianImageFilter.h>

using namespace complex;

namespace cxITKDiscreteGaussianImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKDiscreteGaussianImageFunctor
{
  using VarianceInputArrayType = std::vector<float64>;
  VarianceInputArrayType variance = std::vector<double>(3, 1.0);
  uint32 maximumKernelWidth = 32u;
  using MaximumErrorInputArrayType = std::vector<float64>;
  MaximumErrorInputArrayType maximumError = std::vector<double>(3, 0.01);
  bool useImageSpacing = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::DiscreteGaussianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetVariance(variance.data()); // Set the Variance Filter Property
    filter->SetMaximumKernelWidth(maximumKernelWidth);
    filter->SetMaximumError(maximumError.data()); // Set the MaximumError Filter Property
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKDiscreteGaussianImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::name() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::className() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKDiscreteGaussianImage::uuid() const
{
  return FilterTraits<ITKDiscreteGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKDiscreteGaussianImage::humanName() const
{
  return "ITK Discrete Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDiscreteGaussianImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKDiscreteGaussianImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKDiscreteGaussianImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Variance_Key, "Variance", "", std::vector<double>(3, 1.0), std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<UInt32Parameter>(k_MaximumKernelWidth_Key, "MaximumKernelWidth",
                                                  "Set the kernel to be no wider than MaximumKernelWidth pixels, even if MaximumError demands it. The default is 32 pixels.", 32u));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_MaximumError_Key, "MaximumError", "", std::vector<double>(3, 0.01), std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(
      std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing",
                                      "Set/Get whether or not the filter will use the spacing of the input image in its calculations. Use On to take the image spacing information into account and to "
                                      "specify the Gaussian variance in real world units; use Off to ignore the image spacing and to specify the Gaussian variance in voxel units. Default is On.",
                                      true));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKDiscreteGaussianImage::clone() const
{
  return std::make_unique<ITKDiscreteGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDiscreteGaussianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto variance = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Variance_Key);

  auto maximumKernelWidth = filterArgs.value<uint32>(k_MaximumKernelWidth_Key);
  auto maximumError = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MaximumError_Key);

  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKDiscreteGaussianImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKDiscreteGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto variance = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Variance_Key);

  auto maximumKernelWidth = filterArgs.value<uint32>(k_MaximumKernelWidth_Key);
  auto maximumError = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MaximumError_Key);

  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  const cxITKDiscreteGaussianImage::ITKDiscreteGaussianImageFunctor itkFunctor = {variance, maximumKernelWidth, maximumError, useImageSpacing};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKDiscreteGaussianImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
