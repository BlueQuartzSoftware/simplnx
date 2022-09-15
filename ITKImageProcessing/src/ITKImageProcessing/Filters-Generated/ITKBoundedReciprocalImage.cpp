#include "ITKBoundedReciprocalImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkBoundedReciprocalImageFilter.h>

using namespace complex;

namespace cxITKBoundedReciprocalImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;
template <class PixelT>
using FilterOutputT = double;

struct ITKBoundedReciprocalImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BoundedReciprocalImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    return filter;
  }
};
} // namespace cxITKBoundedReciprocalImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBoundedReciprocalImage::name() const
{
  return FilterTraits<ITKBoundedReciprocalImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBoundedReciprocalImage::className() const
{
  return FilterTraits<ITKBoundedReciprocalImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBoundedReciprocalImage::uuid() const
{
  return FilterTraits<ITKBoundedReciprocalImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBoundedReciprocalImage::humanName() const
{
  return "ITK Bounded Reciprocal Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBoundedReciprocalImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKBoundedReciprocalImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKBoundedReciprocalImage::parameters() const
{
  Parameters params;

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
IFilter::UniquePointer ITKBoundedReciprocalImage::clone() const
{
  return std::make_unique<ITKBoundedReciprocalImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBoundedReciprocalImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKBoundedReciprocalImage::ArrayOptionsT, cxITKBoundedReciprocalImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBoundedReciprocalImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  cxITKBoundedReciprocalImage::ITKBoundedReciprocalImageFunctor itkFunctor = {};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKBoundedReciprocalImage::ArrayOptionsT, cxITKBoundedReciprocalImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
