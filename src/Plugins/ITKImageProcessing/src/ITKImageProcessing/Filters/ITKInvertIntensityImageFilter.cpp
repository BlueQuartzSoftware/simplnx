#include "ITKInvertIntensityImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkInvertIntensityImageFilter.h>

using namespace nx::core;

namespace cxITKInvertIntensityImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKInvertIntensityImageFunctor
{
  float64 maximum = 255;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::InvertIntensityImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetMaximum(maximum);
    return filter;
  }
};
} // namespace cxITKInvertIntensityImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKInvertIntensityImageFilter::name() const
{
  return FilterTraits<ITKInvertIntensityImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKInvertIntensityImageFilter::className() const
{
  return FilterTraits<ITKInvertIntensityImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKInvertIntensityImageFilter::uuid() const
{
  return FilterTraits<ITKInvertIntensityImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKInvertIntensityImageFilter::humanName() const
{
  return "ITK Invert Intensity Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKInvertIntensityImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKInvertIntensityImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKInvertIntensityImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Maximum_Key, "Maximum", "Set/Get the maximum intensity value for the inversion.", 255));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Array Name",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKInvertIntensityImageFilter::clone() const
{
  return std::make_unique<ITKInvertIntensityImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKInvertIntensityImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto maximum = filterArgs.value<float64>(k_Maximum_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKInvertIntensityImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKInvertIntensityImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto maximum = filterArgs.value<float64>(k_Maximum_Key);

  const cxITKInvertIntensityImageFilter::ITKInvertIntensityImageFunctor itkFunctor = {maximum};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKInvertIntensityImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MaximumKey = "Maximum";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKInvertIntensityImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKInvertIntensityImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_MaximumKey, k_Maximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
