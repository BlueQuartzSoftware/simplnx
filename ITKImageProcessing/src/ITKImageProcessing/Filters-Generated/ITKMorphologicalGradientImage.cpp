#include "ITKMorphologicalGradientImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkMorphologicalGradientImageFilter.h>

using namespace complex;

namespace cxITKMorphologicalGradientImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKMorphologicalGradientImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::MorphologicalGradientImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement<InputImageT::ImageDimension>>;
    auto filter = FilterT::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    return filter;
  }
};
} // namespace cxITKMorphologicalGradientImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMorphologicalGradientImage::name() const
{
  return FilterTraits<ITKMorphologicalGradientImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalGradientImage::className() const
{
  return FilterTraits<ITKMorphologicalGradientImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMorphologicalGradientImage::uuid() const
{
  return FilterTraits<ITKMorphologicalGradientImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalGradientImage::humanName() const
{
  return "ITK Morphological Gradient Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMorphologicalGradientImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKMorphologicalGradientImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKMorphologicalGradientImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "KernelRadius", "", std::vector<uint32>(3, 1), std::vector<std::string>(3)));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "KernelType", "", static_cast<uint64>(itk::simple::sitkBall), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));

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
IFilter::UniquePointer ITKMorphologicalGradientImage::clone() const
{
  return std::make_unique<ITKMorphologicalGradientImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMorphologicalGradientImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMorphologicalGradientImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMorphologicalGradientImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));

  cxITKMorphologicalGradientImage::ITKMorphologicalGradientImageFunctor itkFunctor = {kernelRadius, kernelType};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKMorphologicalGradientImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
