#include "ITKBinaryThinningImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkBinaryThinningImageFilter.h>

using namespace complex;

namespace cxITKBinaryThinningImage
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;

struct ITKBinaryThinningImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BinaryThinningImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    return filter;
  }
};
} // namespace cxITKBinaryThinningImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinaryThinningImage::name() const
{
  return FilterTraits<ITKBinaryThinningImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryThinningImage::className() const
{
  return FilterTraits<ITKBinaryThinningImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryThinningImage::uuid() const
{
  return FilterTraits<ITKBinaryThinningImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryThinningImage::humanName() const
{
  return "ITK Binary Thinning Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryThinningImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKBinaryThinningImage", "ITKBinaryMathematicalMorphology", "BinaryMathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryThinningImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryThinningImage::clone() const
{
  return std::make_unique<ITKBinaryThinningImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryThinningImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinaryThinningImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryThinningImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  cxITKBinaryThinningImage::ITKBinaryThinningImageFunctor itkFunctor = {};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKBinaryThinningImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
