#include "ITKRegionalMaximaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkRegionalMaximaImageFilter.h>

using namespace complex;

namespace cxITKRegionalMaximaImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = uint32;

struct ITKRegionalMaximaImageFunctor
{
  float64 backgroundValue = 0.0;
  float64 foregroundValue = 1.0;
  bool fullyConnected = false;
  bool flatIsMaxima = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::RegionalMaximaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetBackgroundValue(backgroundValue);
    filter->SetForegroundValue(foregroundValue);
    filter->SetFullyConnected(fullyConnected);
    filter->SetFlatIsMaxima(flatIsMaxima);
    return filter;
  }
};
} // namespace cxITKRegionalMaximaImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKRegionalMaximaImage::name() const
{
  return FilterTraits<ITKRegionalMaximaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKRegionalMaximaImage::className() const
{
  return FilterTraits<ITKRegionalMaximaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKRegionalMaximaImage::uuid() const
{
  return FilterTraits<ITKRegionalMaximaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKRegionalMaximaImage::humanName() const
{
  return "ITK Regional Maxima Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKRegionalMaximaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKRegionalMaximaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKRegionalMaximaImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insert(std::make_unique<BoolParameter>(k_FlatIsMaxima_Key, "FlatIsMaxima", "", true));

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
IFilter::UniquePointer ITKRegionalMaximaImage::clone() const
{
  return std::make_unique<ITKRegionalMaximaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKRegionalMaximaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto flatIsMaxima = filterArgs.value<bool>(k_FlatIsMaxima_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKRegionalMaximaImage::ArrayOptionsT, cxITKRegionalMaximaImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKRegionalMaximaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto flatIsMaxima = filterArgs.value<bool>(k_FlatIsMaxima_Key);

  cxITKRegionalMaximaImage::ITKRegionalMaximaImageFunctor itkFunctor = {backgroundValue, foregroundValue, fullyConnected, flatIsMaxima};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKRegionalMaximaImage::ArrayOptionsT, cxITKRegionalMaximaImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
