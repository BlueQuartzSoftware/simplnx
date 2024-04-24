#include "ITKNormalizeToConstantImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkNormalizeToConstantImageFilter.h>

using namespace nx::core;

namespace cxITKNormalizeToConstantImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;
template <class PixelT>
using FilterOutputType = double;

struct ITKNormalizeToConstantImageFunctor
{
  float64 constant = 1.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::NormalizeToConstantImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetConstant(constant);
    return filter;
  }
};
} // namespace cxITKNormalizeToConstantImage

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKNormalizeToConstantImage::name() const
{
  return FilterTraits<ITKNormalizeToConstantImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKNormalizeToConstantImage::className() const
{
  return FilterTraits<ITKNormalizeToConstantImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKNormalizeToConstantImage::uuid() const
{
  return FilterTraits<ITKNormalizeToConstantImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKNormalizeToConstantImage::humanName() const
{
  return "ITK Normalize To Constant Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKNormalizeToConstantImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKNormalizeToConstantImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKNormalizeToConstantImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Constant_Key, "Constant", "Set/get the normalization constant.", 1.0));

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
IFilter::UniquePointer ITKNormalizeToConstantImage::clone() const
{
  return std::make_unique<ITKNormalizeToConstantImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKNormalizeToConstantImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto constant = filterArgs.value<float64>(k_Constant_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKNormalizeToConstantImage::ArrayOptionsType, cxITKNormalizeToConstantImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKNormalizeToConstantImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto constant = filterArgs.value<float64>(k_Constant_Key);

  const cxITKNormalizeToConstantImage::ITKNormalizeToConstantImageFunctor itkFunctor = {constant};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKNormalizeToConstantImage::ArrayOptionsType, cxITKNormalizeToConstantImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                        shouldCancel);
}
} // namespace nx::core
