#include "ITKSinImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include <itkSinImageFilter.h>

using namespace complex;

namespace
{
struct ITKSinImageFilterCreationFunctor
{
  template <class InputImageType, class OutputImageType>
  auto operator()() const
  {
    using FilterType = itk::SinImageFilter<InputImageType, OutputImageType>;
    return FilterType::New();
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSinImage::name() const
{
  return FilterTraits<ITKSinImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKSinImage::className() const
{
  return FilterTraits<ITKSinImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSinImage::uuid() const
{
  return FilterTraits<ITKSinImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSinImage::humanName() const
{
  return "ITK::Sin Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSinImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKIntensityTransformation"};
}

//------------------------------------------------------------------------------
Parameters ITKSinImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSinImage::clone() const
{
  return std::make_unique<ITKSinImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSinImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto inputArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  Result<OutputActions> result = ITK::DataCheck(dataStructure, inputArrayPath, imageGeomPath, outputArrayPath);

  return {std::move(result)};
}

//------------------------------------------------------------------------------
Result<> ITKSinImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto inputArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ITK::Execute(dataStructure, inputArrayPath, imageGeomPath, outputArrayPath, ITKSinImageFilterCreationFunctor{});
}
} // namespace complex
