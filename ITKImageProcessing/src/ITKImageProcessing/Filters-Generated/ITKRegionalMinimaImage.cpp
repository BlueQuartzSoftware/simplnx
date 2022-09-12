#include "ITKRegionalMinimaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkRegionalMinimaImageFilter.h>

using namespace complex;

namespace cxITKRegionalMinimaImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = uint32;

struct ITKRegionalMinimaImageFunctor
{
  float64 backgroundValue = 0.0;
  float64 foregroundValue = 1.0;
  bool fullyConnected = false;
  bool flatIsMinima = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::RegionalMinimaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetBackgroundValue(backgroundValue);
    filter->SetForegroundValue(foregroundValue);
    filter->SetFullyConnected(fullyConnected);
    filter->SetFlatIsMinima(flatIsMinima);
    return filter;
  }
};
} // namespace cxITKRegionalMinimaImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKRegionalMinimaImage::name() const
{
  return FilterTraits<ITKRegionalMinimaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKRegionalMinimaImage::className() const
{
  return FilterTraits<ITKRegionalMinimaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKRegionalMinimaImage::uuid() const
{
  return FilterTraits<ITKRegionalMinimaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKRegionalMinimaImage::humanName() const
{
  return "ITK Regional Minima Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKRegionalMinimaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKRegionalMinimaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKRegionalMinimaImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insert(std::make_unique<BoolParameter>(k_FlatIsMinima_Key, "FlatIsMinima", "", true));

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
IFilter::UniquePointer ITKRegionalMinimaImage::clone() const
{
  return std::make_unique<ITKRegionalMinimaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKRegionalMinimaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto flatIsMinima = filterArgs.value<bool>(k_FlatIsMinima_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKRegionalMinimaImage::ArrayOptionsT, cxITKRegionalMinimaImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKRegionalMinimaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto flatIsMinima = filterArgs.value<bool>(k_FlatIsMinima_Key);

  cxITKRegionalMinimaImage::ITKRegionalMinimaImageFunctor itkFunctor = {backgroundValue, foregroundValue, fullyConnected, flatIsMinima};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKRegionalMinimaImage::ArrayOptionsT, cxITKRegionalMinimaImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
