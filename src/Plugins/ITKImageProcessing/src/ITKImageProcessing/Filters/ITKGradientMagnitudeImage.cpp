#include "ITKGradientMagnitudeImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkGradientMagnitudeImageFilter.h>

using namespace complex;

namespace cxITKGradientMagnitudeImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKGradientMagnitudeImageFunctor
{
  bool useImageSpacing = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::GradientMagnitudeImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
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
  return {className(), "ITKImageProcessing", "ITKGradientMagnitudeImage", "ITKImageGradient", "ImageGradient"};
}

//------------------------------------------------------------------------------
Parameters ITKGradientMagnitudeImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing",
                                                "Set/Get whether or not the filter will use the spacing of the input image in the computation of the derivatives. Use On to compute the gradient in "
                                                "physical space; use Off to ignore image spacing and to compute the gradient in isotropic voxel space. Default is On.",
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
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKGradientMagnitudeImage::ArrayOptionsType, cxITKGradientMagnitudeImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGradientMagnitudeImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  cxITKGradientMagnitudeImage::ITKGradientMagnitudeImageFunctor itkFunctor = {useImageSpacing};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKGradientMagnitudeImage::ArrayOptionsType, cxITKGradientMagnitudeImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                    shouldCancel);
}
} // namespace complex
