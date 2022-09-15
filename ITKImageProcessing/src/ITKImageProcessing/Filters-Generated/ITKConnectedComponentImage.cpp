#include "ITKConnectedComponentImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkConnectedComponentImageFilter.h>

using namespace complex;

namespace cxITKConnectedComponentImage
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = uint32;

struct ITKConnectedComponentImageFunctor
{
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ConnectedComponentImageFilter<InputImageT, OutputImageT, itk::Image<uint8_t, InputImageT::ImageDimension>>;
    auto filter = FilterT::New();
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKConnectedComponentImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKConnectedComponentImage::name() const
{
  return FilterTraits<ITKConnectedComponentImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKConnectedComponentImage::className() const
{
  return FilterTraits<ITKConnectedComponentImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKConnectedComponentImage::uuid() const
{
  return FilterTraits<ITKConnectedComponentImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKConnectedComponentImage::humanName() const
{
  return "ITK Connected Component Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKConnectedComponentImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKConnectedComponentImage", "ITKConnectedComponents", "ConnectedComponents"};
}

//------------------------------------------------------------------------------
Parameters ITKConnectedComponentImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

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
IFilter::UniquePointer ITKConnectedComponentImage::clone() const
{
  return std::make_unique<ITKConnectedComponentImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKConnectedComponentImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKConnectedComponentImage::ArrayOptionsT, cxITKConnectedComponentImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKConnectedComponentImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  cxITKConnectedComponentImage::ITKConnectedComponentImageFunctor itkFunctor = {fullyConnected};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKConnectedComponentImage::ArrayOptionsT, cxITKConnectedComponentImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
