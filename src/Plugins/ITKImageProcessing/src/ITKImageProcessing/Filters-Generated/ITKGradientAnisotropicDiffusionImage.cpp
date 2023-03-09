#include "ITKGradientAnisotropicDiffusionImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkGradientAnisotropicDiffusionImageFilter.h>

using namespace complex;

namespace cxITKGradientAnisotropicDiffusionImage
{
using ArrayOptionsT = ITK::FloatingScalarPixelIdTypeList;

struct ITKGradientAnisotropicDiffusionImageFunctor
{
  float64 timeStep = 0.125;
  float64 conductanceParameter = 3;
  uint32 conductanceScalingUpdateInterval = 1u;
  uint32 numberOfIterations = 5u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::GradientAnisotropicDiffusionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetTimeStep(timeStep);
    filter->SetConductanceParameter(conductanceParameter);
    filter->SetConductanceScalingUpdateInterval(conductanceScalingUpdateInterval);
    filter->SetNumberOfIterations(numberOfIterations);
    return filter;
  }
};
} // namespace cxITKGradientAnisotropicDiffusionImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGradientAnisotropicDiffusionImage::name() const
{
  return FilterTraits<ITKGradientAnisotropicDiffusionImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKGradientAnisotropicDiffusionImage::className() const
{
  return FilterTraits<ITKGradientAnisotropicDiffusionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGradientAnisotropicDiffusionImage::uuid() const
{
  return FilterTraits<ITKGradientAnisotropicDiffusionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGradientAnisotropicDiffusionImage::humanName() const
{
  return "ITK Gradient Anisotropic Diffusion Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGradientAnisotropicDiffusionImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKGradientAnisotropicDiffusionImage", "ITKAnisotropicSmoothing", "AnisotropicSmoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKGradientAnisotropicDiffusionImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "TimeStep", "", 0.125));
  params.insert(std::make_unique<Float64Parameter>(k_ConductanceParameter_Key, "ConductanceParameter", "", 3));
  params.insert(std::make_unique<UInt32Parameter>(k_ConductanceScalingUpdateInterval_Key, "ConductanceScalingUpdateInterval", "", 1u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 5u));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetFloatingScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKGradientAnisotropicDiffusionImage::clone() const
{
  return std::make_unique<ITKGradientAnisotropicDiffusionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGradientAnisotropicDiffusionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto conductanceParameter = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto conductanceScalingUpdateInterval = filterArgs.value<uint32>(k_ConductanceScalingUpdateInterval_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKGradientAnisotropicDiffusionImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGradientAnisotropicDiffusionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto timeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto conductanceParameter = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto conductanceScalingUpdateInterval = filterArgs.value<uint32>(k_ConductanceScalingUpdateInterval_Key);
  auto numberOfIterations = filterArgs.value<uint32>(k_NumberOfIterations_Key);

  cxITKGradientAnisotropicDiffusionImage::ITKGradientAnisotropicDiffusionImageFunctor itkFunctor = {timeStep, conductanceParameter, conductanceScalingUpdateInterval, numberOfIterations};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKGradientAnisotropicDiffusionImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
