#include "ITKSinImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkSinImageFilter.h>

using namespace nx::core;

namespace cxITKSinImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKSinImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::SinImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    return filter;
  }
};
} // namespace cxITKSinImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKSinImageFilter::name() const
{
  return FilterTraits<ITKSinImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKSinImageFilter::className() const
{
  return FilterTraits<ITKSinImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSinImageFilter::uuid() const
{
  return FilterTraits<ITKSinImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSinImageFilter::humanName() const
{
  return "ITK Sin Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSinImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKSinImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKSinImageFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSinImageFilter::clone() const
{
  return std::make_unique<ITKSinImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSinImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSinImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSinImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const cxITKSinImageFilter::ITKSinImageFunctor itkFunctor = {};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKSinImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKSinImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKSinImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
