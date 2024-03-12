#include "ITKMinMaxCurvatureFlowImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkMinMaxCurvatureFlowImageFilter.h>

using namespace nx::core;

namespace cxITKMinMaxCurvatureFlowImage
{
using ArrayOptionsType = ITK::FloatingScalarPixelIdTypeList;

struct ITKMinMaxCurvatureFlowImageFunctor
{
  using IntermediateType = float64;

  float64 timeStep = 0.05;
  uint32 numberOfIterations = 5u;
  int32 stencilRadius = 2;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MinMaxCurvatureFlowImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetTimeStep(timeStep);
    filter->SetNumberOfIterations(numberOfIterations);
    filter->SetStencilRadius(stencilRadius);
    return filter;
  }
};
} // namespace cxITKMinMaxCurvatureFlowImage

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMinMaxCurvatureFlowImage::name() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMinMaxCurvatureFlowImage::className() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMinMaxCurvatureFlowImage::uuid() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMinMaxCurvatureFlowImage::humanName() const
{
  return "ITK Min Max Curvature Flow Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMinMaxCurvatureFlowImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMinMaxCurvatureFlowImage", "ITKCurvatureFlow", "CurvatureFlow"};
}

//------------------------------------------------------------------------------
Parameters ITKMinMaxCurvatureFlowImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "TimeStep", "", 0.05));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 5u));
  params.insert(std::make_unique<Int32Parameter>(k_StencilRadius_Key, "StencilRadius", "Set/Get the stencil radius.", 2));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetFloatingScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMinMaxCurvatureFlowImage::clone() const
{
  return std::make_unique<ITKMinMaxCurvatureFlowImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMinMaxCurvatureFlowImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);
  auto stencilRadius = filterArgs.value<int32>(k_StencilRadius_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMinMaxCurvatureFlowImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMinMaxCurvatureFlowImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);
  auto stencilRadius = filterArgs.value<int32>(k_StencilRadius_Key);

  const cxITKMinMaxCurvatureFlowImage::ITKMinMaxCurvatureFlowImageFunctor itkFunctor = {timeStep, numberOfIterations, stencilRadius};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKMinMaxCurvatureFlowImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
