#include "ITKCurvatureFlowImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkCurvatureFlowImageFilter.h>

using namespace complex;

namespace cxITKCurvatureFlowImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = double;

struct ITKCurvatureFlowImageFunctor
{
  float64 timeStep = 0.05;
  uint32 numberOfIterations = 5u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::CurvatureFlowImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetTimeStep(timeStep);
    filter->SetNumberOfIterations(numberOfIterations);
    return filter;
  }
};
} // namespace cxITKCurvatureFlowImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKCurvatureFlowImage::name() const
{
  return FilterTraits<ITKCurvatureFlowImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKCurvatureFlowImage::className() const
{
  return FilterTraits<ITKCurvatureFlowImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKCurvatureFlowImage::uuid() const
{
  return FilterTraits<ITKCurvatureFlowImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKCurvatureFlowImage::humanName() const
{
  return "ITK Curvature Flow Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKCurvatureFlowImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKCurvatureFlowImage", "ITKCurvatureFlow", "CurvatureFlow"};
}

//------------------------------------------------------------------------------
Parameters ITKCurvatureFlowImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "TimeStep", "", 0.05));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 5u));

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
IFilter::UniquePointer ITKCurvatureFlowImage::clone() const
{
  return std::make_unique<ITKCurvatureFlowImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKCurvatureFlowImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKCurvatureFlowImage::ArrayOptionsT, cxITKCurvatureFlowImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKCurvatureFlowImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  cxITKCurvatureFlowImage::ITKCurvatureFlowImageFunctor itkFunctor = {timeStep, numberOfIterations};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKCurvatureFlowImage::ArrayOptionsT, cxITKCurvatureFlowImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
