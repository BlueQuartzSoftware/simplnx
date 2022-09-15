#include "ITKSquareImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkSquareImageFilter.h>

using namespace complex;

namespace cxITKSquareImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKSquareImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::SquareImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    return filter;
  }
};
} // namespace cxITKSquareImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSquareImage::name() const
{
  return FilterTraits<ITKSquareImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKSquareImage::className() const
{
  return FilterTraits<ITKSquareImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSquareImage::uuid() const
{
  return FilterTraits<ITKSquareImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSquareImage::humanName() const
{
  return "ITK Square Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSquareImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKSquareImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKSquareImage::parameters() const
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
IFilter::UniquePointer ITKSquareImage::clone() const
{
  return std::make_unique<ITKSquareImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSquareImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSquareImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSquareImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  cxITKSquareImage::ITKSquareImageFunctor itkFunctor = {};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKSquareImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
