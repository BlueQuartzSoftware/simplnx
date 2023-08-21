#include "ITKOpeningByReconstructionImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkOpeningByReconstructionImageFilter.h>

using namespace complex;

namespace cxITKOpeningByReconstructionImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKOpeningByReconstructionImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  bool fullyConnected = false;
  bool preserveIntensities = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::OpeningByReconstructionImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement<InputImageT::ImageDimension>>;
    auto filter = FilterT::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetFullyConnected(fullyConnected);
    filter->SetPreserveIntensities(preserveIntensities);
    return filter;
  }
};
} // namespace cxITKOpeningByReconstructionImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKOpeningByReconstructionImage::name() const
{
  return FilterTraits<ITKOpeningByReconstructionImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKOpeningByReconstructionImage::className() const
{
  return FilterTraits<ITKOpeningByReconstructionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKOpeningByReconstructionImage::uuid() const
{
  return FilterTraits<ITKOpeningByReconstructionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKOpeningByReconstructionImage::humanName() const
{
  return "ITK Opening By Reconstruction Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKOpeningByReconstructionImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKOpeningByReconstructionImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKOpeningByReconstructionImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "KernelRadius", "", std::vector<uint32>(3, 1), std::vector<std::string>(3)));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "KernelType", "", static_cast<uint64>(itk::simple::sitkBall), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<BoolParameter>(
      k_FullyConnected_Key, "FullyConnected",
      "Whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. For objects that are 1 pixel wide, use FullyConnectedOn.", false));
  params.insert(std::make_unique<BoolParameter>(
      k_PreserveIntensities_Key, "PreserveIntensities",
      "Whether the original intensities of the image retained for those pixels unaffected by the opening by reconstrcution. If Off, the output pixel contrast will be reduced.", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKOpeningByReconstructionImage::clone() const
{
  return std::make_unique<ITKOpeningByReconstructionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKOpeningByReconstructionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto preserveIntensities = filterArgs.value<bool>(k_PreserveIntensities_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKOpeningByReconstructionImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKOpeningByReconstructionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto preserveIntensities = filterArgs.value<bool>(k_PreserveIntensities_Key);

  cxITKOpeningByReconstructionImage::ITKOpeningByReconstructionImageFunctor itkFunctor = {kernelRadius, kernelType, fullyConnected, preserveIntensities};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKOpeningByReconstructionImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
