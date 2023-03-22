#include "ImageContouringFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ImageContouring.hpp"

#include "complex/Common/Types.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImageContouringFilter::name() const
{
  return FilterTraits<ImageContouringFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImageContouringFilter::className() const
{
  return FilterTraits<ImageContouringFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ImageContouringFilter::uuid() const
{
  return FilterTraits<ImageContouringFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImageContouringFilter::humanName() const
{
  return "Image Contouring";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImageContouringFilter::defaultTags() const
{
  return {"Contouring", "Image Geometry"};
}

//------------------------------------------------------------------------------
Parameters ImageContouringFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_IsoVal_Key, "Contour Value", "The value to contour on", 1.0));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_SelectedDataArray_Key, "Data Array to Contour", "This is the data that will be checked for the contouring iso value", DataPath{}, GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_NewTriangleGeometry_Key, "Name of Output Triangle Geometry", "This is where the contouring line will be stored", "Contouring Geometry"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImageContouringFilter::clone() const
{
  return std::make_unique<ImageContouringFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImageContouringFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImageContouringFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  ImageContouringInputValues inputValues;

  inputValues.imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.contouringArrayPath = filterArgs.value<DataPath>(k_SelectedDataArray_Key);
  inputValues.triangleGeomPath = filterArgs.value<DataPath>(k_NewTriangleGeometry_Key);
  inputValues.isoVal = filterArgs.value<float64>(k_IsoVal_Key);

  return ImageContouring(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
