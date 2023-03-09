#include "ITKHMinimaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkHMinimaImageFilter.h>

using namespace complex;

namespace cxITKHMinimaImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKHMinimaImageFunctor
{
  float64 height = 2.0;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::HMinimaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetHeight(height);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKHMinimaImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKHMinimaImage::name() const
{
  return FilterTraits<ITKHMinimaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKHMinimaImage::className() const
{
  return FilterTraits<ITKHMinimaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKHMinimaImage::uuid() const
{
  return FilterTraits<ITKHMinimaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKHMinimaImage::humanName() const
{
  return "ITK H Minima Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKHMinimaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKHMinimaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKHMinimaImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Height_Key, "Height", "", 2.0));
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
IFilter::UniquePointer ITKHMinimaImage::clone() const
{
  return std::make_unique<ITKHMinimaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKHMinimaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKHMinimaImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKHMinimaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  cxITKHMinimaImage::ITKHMinimaImageFunctor itkFunctor = {height, fullyConnected};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKHMinimaImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
