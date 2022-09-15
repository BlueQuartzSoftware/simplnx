#include "ITKValuedRegionalMaximaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkValuedRegionalMaximaImageFilter.h>

using namespace complex;

namespace cxITKValuedRegionalMaximaImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKValuedRegionalMaximaImageFunctor
{
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ValuedRegionalMaximaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKValuedRegionalMaximaImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImage::name() const
{
  return FilterTraits<ITKValuedRegionalMaximaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImage::className() const
{
  return FilterTraits<ITKValuedRegionalMaximaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKValuedRegionalMaximaImage::uuid() const
{
  return FilterTraits<ITKValuedRegionalMaximaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImage::humanName() const
{
  return "ITK Valued Regional Maxima Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKValuedRegionalMaximaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKValuedRegionalMaximaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKValuedRegionalMaximaImage::parameters() const
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
IFilter::UniquePointer ITKValuedRegionalMaximaImage::clone() const
{
  return std::make_unique<ITKValuedRegionalMaximaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKValuedRegionalMaximaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKValuedRegionalMaximaImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKValuedRegionalMaximaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  cxITKValuedRegionalMaximaImage::ITKValuedRegionalMaximaImageFunctor itkFunctor = {fullyConnected};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKValuedRegionalMaximaImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
