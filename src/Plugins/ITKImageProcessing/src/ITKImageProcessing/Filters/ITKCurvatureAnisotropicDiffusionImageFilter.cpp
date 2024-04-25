#include "ITKCurvatureAnisotropicDiffusionImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <ITKCurvatureAnisotropicDiffusionImageFilter.h>

using namespace nx::core;

namespace cxITKCurvatureAnisotropicDiffusionImageFilter
{
using ArrayOptionsType = ITK::FloatingScalarPixelIdTypeList;

struct ITKCurvatureAnisotropicDiffusionImageFilterFunctor
{
  float64 timeStep = 0.0625;
  float64 conductanceParameter = 3.0;
  uint32 conductanceScalingUpdateInterval = 1u;
  uint32 numberOfIterations = 5u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::CurvatureAnisotropicDiffusionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetTimeStep(timeStep);
    filter->SetConductanceParameter(conductanceParameter);
    filter->SetConductanceScalingUpdateInterval(conductanceScalingUpdateInterval);
    filter->SetNumberOfIterations(numberOfIterations);
    return filter;
  }
};
} // namespace cxITKCurvatureAnisotropicDiffusionImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKCurvatureAnisotropicDiffusionImageFilter::name() const
{
  return FilterTraits<ITKCurvatureAnisotropicDiffusionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKCurvatureAnisotropicDiffusionImageFilter::className() const
{
  return FilterTraits<ITKCurvatureAnisotropicDiffusionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKCurvatureAnisotropicDiffusionImageFilter::uuid() const
{
  return FilterTraits<ITKCurvatureAnisotropicDiffusionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKCurvatureAnisotropicDiffusionImageFilter::humanName() const
{
  return "ITK Curvature Anisotropic Diffusion Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKCurvatureAnisotropicDiffusionImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKCurvatureAnisotropicDiffusionImageFilter", "ITKAnisotropicSmoothing", "AnisotropicSmoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKCurvatureAnisotropicDiffusionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "Time Step", "The time step to be used for each iteration.", 0.0625));
  params.insert(std::make_unique<Float64Parameter>(k_ConductanceParameter_Key, "Conductance Parameter",
                                                   "The conductance parameter controls the sensitivity of the conductance term in the basic anisotropic diffusion equation", 3.0));
  params.insert(std::make_unique<UInt32Parameter>(k_ConductanceScalingUpdateInterval_Key, "Conductance Scaling Update Interval", "The interval between conductance updates.", 1u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "Number Of Iterations",
                                                  "Specifies the number of iterations (time-step updates) that the solver will perform to produce a solution image", 5u));

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
IFilter::UniquePointer ITKCurvatureAnisotropicDiffusionImageFilter::clone() const
{
  return std::make_unique<ITKCurvatureAnisotropicDiffusionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKCurvatureAnisotropicDiffusionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto conductanceParameter = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto conductanceScalingUpdateInterval = filterArgs.value<uint32>(k_ConductanceScalingUpdateInterval_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKCurvatureAnisotropicDiffusionImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKCurvatureAnisotropicDiffusionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto conductanceParameter = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto conductanceScalingUpdateInterval = filterArgs.value<uint32>(k_ConductanceScalingUpdateInterval_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  const cxITKCurvatureAnisotropicDiffusionImageFilter::ITKCurvatureAnisotropicDiffusionImageFilterFunctor itkFunctor = {timeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKCurvatureAnisotropicDiffusionImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
