#include "ITKGrayscaleGrindPeakImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkGrayscaleGrindPeakImageFilter.h>

using namespace complex;

namespace cxITKGrayscaleGrindPeakImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKGrayscaleGrindPeakImageFunctor
{
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::GrayscaleGrindPeakImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKGrayscaleGrindPeakImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGrayscaleGrindPeakImage::name() const
{
  return FilterTraits<ITKGrayscaleGrindPeakImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKGrayscaleGrindPeakImage::className() const
{
  return FilterTraits<ITKGrayscaleGrindPeakImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGrayscaleGrindPeakImage::uuid() const
{
  return FilterTraits<ITKGrayscaleGrindPeakImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGrayscaleGrindPeakImage::humanName() const
{
  return "ITK Grayscale Grind Peak Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGrayscaleGrindPeakImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKGrayscaleGrindPeakImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKGrayscaleGrindPeakImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

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
IFilter::UniquePointer ITKGrayscaleGrindPeakImage::clone() const
{
  return std::make_unique<ITKGrayscaleGrindPeakImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGrayscaleGrindPeakImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKGrayscaleGrindPeakImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGrayscaleGrindPeakImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  cxITKGrayscaleGrindPeakImage::ITKGrayscaleGrindPeakImageFunctor itkFunctor = {fullyConnected};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKGrayscaleGrindPeakImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
