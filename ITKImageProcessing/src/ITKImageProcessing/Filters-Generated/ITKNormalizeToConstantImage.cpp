#include "ITKNormalizeToConstantImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkNormalizeToConstantImageFilter.h>

using namespace complex;

namespace cxITKNormalizeToConstantImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;
template <class PixelT>
using FilterOutputT = double;

struct ITKNormalizeToConstantImageFunctor
{
  float64 constant = 1.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::NormalizeToConstantImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetConstant(constant);
    return filter;
  }
};
} // namespace cxITKNormalizeToConstantImage

namespace complex
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
  return {"ITKImageProcessing", "ITKNormalizeToConstantImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKNormalizeToConstantImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Constant_Key, "Constant", "", 1.0));

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
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto constant = filterArgs.value<float64>(k_Constant_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKNormalizeToConstantImage::ArrayOptionsT, cxITKNormalizeToConstantImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKNormalizeToConstantImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto constant = filterArgs.value<float64>(k_Constant_Key);

  cxITKNormalizeToConstantImage::ITKNormalizeToConstantImageFunctor itkFunctor = {constant};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKNormalizeToConstantImage::ArrayOptionsT, cxITKNormalizeToConstantImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
