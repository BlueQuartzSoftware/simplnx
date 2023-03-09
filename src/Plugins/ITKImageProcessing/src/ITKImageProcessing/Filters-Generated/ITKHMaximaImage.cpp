#include "ITKHMaximaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkHMaximaImageFilter.h>

using namespace complex;

namespace cxITKHMaximaImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKHMaximaImageFunctor
{
  float64 height = 2.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::HMaximaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetHeight(height);
    return filter;
  }
};
} // namespace cxITKHMaximaImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKHMaximaImage::name() const
{
  return FilterTraits<ITKHMaximaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKHMaximaImage::className() const
{
  return FilterTraits<ITKHMaximaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKHMaximaImage::uuid() const
{
  return FilterTraits<ITKHMaximaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKHMaximaImage::humanName() const
{
  return "ITK H Maxima Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKHMaximaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKHMaximaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKHMaximaImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Height_Key, "Height", "", 2.0));

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
IFilter::UniquePointer ITKHMaximaImage::clone() const
{
  return std::make_unique<ITKHMaximaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKHMaximaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKHMaximaImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKHMaximaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto height = filterArgs.value<float64>(k_Height_Key);

  cxITKHMaximaImage::ITKHMaximaImageFunctor itkFunctor = {height};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKHMaximaImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
