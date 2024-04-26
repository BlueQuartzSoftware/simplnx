#include "ITKMinMaxCurvatureFlowImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <ITKMinMaxCurvatureFlowImageFilter.h>

using namespace nx::core;

namespace cxITKMinMaxCurvatureFlowImageFilter
{
using ArrayOptionsType = ITK::FloatingScalarPixelIdTypeList;

struct ITKMinMaxCurvatureFlowImageFilterFunctor
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
} // namespace cxITKMinMaxCurvatureFlowImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMinMaxCurvatureFlowImageFilter::name() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMinMaxCurvatureFlowImageFilter::className() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMinMaxCurvatureFlowImageFilter::uuid() const
{
  return FilterTraits<ITKMinMaxCurvatureFlowImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMinMaxCurvatureFlowImageFilter::humanName() const
{
  return "ITK Min Max Curvature Flow Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMinMaxCurvatureFlowImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMinMaxCurvatureFlowImageFilter", "ITKCurvatureFlow", "CurvatureFlow"};
}

//------------------------------------------------------------------------------
Parameters ITKMinMaxCurvatureFlowImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "Time Step", "The time step to be used for each iteration.", 0.05));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "Number Of Iterations",
                                                  "Specifies the number of iterations (time-step updates) that the solver will perform to produce a solution image", 5u));
  params.insert(std::make_unique<Int32Parameter>(k_StencilRadius_Key, "Stencil Radius", "Set/Get the stencil radius.", 2));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetFloatingScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMinMaxCurvatureFlowImageFilter::clone() const
{
  return std::make_unique<ITKMinMaxCurvatureFlowImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMinMaxCurvatureFlowImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);
  auto stencilRadius = filterArgs.value<int32>(k_StencilRadius_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMinMaxCurvatureFlowImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMinMaxCurvatureFlowImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);
  auto stencilRadius = filterArgs.value<int32>(k_StencilRadius_Key);

  const cxITKMinMaxCurvatureFlowImageFilter::ITKMinMaxCurvatureFlowImageFilterFunctor itkFunctor = {timeStep, numberOfIterations, stencilRadius};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKMinMaxCurvatureFlowImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
