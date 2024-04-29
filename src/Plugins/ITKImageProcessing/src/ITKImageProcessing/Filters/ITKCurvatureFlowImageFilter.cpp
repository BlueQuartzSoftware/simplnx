#include "ITKCurvatureFlowImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkCurvatureFlowImageFilter.h>

using namespace nx::core;

namespace cxITKCurvatureFlowImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKCurvatureFlowImageFilterFunctor
{
  using IntermediateType = float64;

  float64 timeStep = 0.05;
  uint32 numberOfIterations = 5u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::CurvatureFlowImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetTimeStep(timeStep);
    filter->SetNumberOfIterations(numberOfIterations);
    return filter;
  }
};
} // namespace cxITKCurvatureFlowImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKCurvatureFlowImageFilter::name() const
{
  return FilterTraits<ITKCurvatureFlowImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKCurvatureFlowImageFilter::className() const
{
  return FilterTraits<ITKCurvatureFlowImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKCurvatureFlowImageFilter::uuid() const
{
  return FilterTraits<ITKCurvatureFlowImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKCurvatureFlowImageFilter::humanName() const
{
  return "ITK Curvature Flow Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKCurvatureFlowImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKCurvatureFlowImageFilter", "ITKCurvatureFlow", "CurvatureFlow"};
}

//------------------------------------------------------------------------------
Parameters ITKCurvatureFlowImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "Time Step", "The timestep between iteration updates", 0.05));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "Number Of Iterations", "The number of update iterations ", 5u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKCurvatureFlowImageFilter::clone() const
{
  return std::make_unique<ITKCurvatureFlowImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKCurvatureFlowImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKCurvatureFlowImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKCurvatureFlowImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  const cxITKCurvatureFlowImageFilter::ITKCurvatureFlowImageFilterFunctor itkFunctor = {timeStep, numberOfIterations};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKCurvatureFlowImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
