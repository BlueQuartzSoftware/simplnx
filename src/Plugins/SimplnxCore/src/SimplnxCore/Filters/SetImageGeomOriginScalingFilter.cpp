#include "SetImageGeomOriginScalingFilter.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <fmt/format.h>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <optional>

namespace nx::core
{

//------------------------------------------------------------------------------
std::string SetImageGeomOriginScalingFilter::name() const
{
  return FilterTraits<SetImageGeomOriginScalingFilter>::name;
}

//------------------------------------------------------------------------------
std::string SetImageGeomOriginScalingFilter::className() const
{
  return FilterTraits<SetImageGeomOriginScalingFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SetImageGeomOriginScalingFilter::uuid() const
{
  return FilterTraits<SetImageGeomOriginScalingFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SetImageGeomOriginScalingFilter::humanName() const
{
  return "Set Origin & Spacing (Image Geom)";
}

//------------------------------------------------------------------------------
std::vector<std::string> SetImageGeomOriginScalingFilter::defaultTags() const
{
  return {className(), "Image Geometry", "Shift", "Spacing", "Mutate", "Origin"};
}

//------------------------------------------------------------------------------
Parameters SetImageGeomOriginScalingFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "Path to the target ImageGeom", DataPath(), std::set{IGeometry::Type::Image}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Set Origin", "Specifies if the origin should be changed", true));
  params.insert(
      std::make_unique<BoolParameter>(k_CenterOrigin_Key, "Put Input Origin at the Center of Geometry", "Specifies if the origin should be aligned with the corner (false) or center (true)", false));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Origin_Key, "Origin  (Physical Units)", "Specifies the new origin values in physical units.", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing_Key, "Set Spacing", "Specifies if the spacing should be changed", true));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing (Physical Units)", "Specifies the new spacing values in physical units.", std::vector<float64>{1, 1, 1},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeOrigin_Key, k_CenterOrigin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeSpacing_Key, k_Spacing_Key, std::make_any<bool>(true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SetImageGeomOriginScalingFilter::clone() const
{
  return std::make_unique<SetImageGeomOriginScalingFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SetImageGeomOriginScalingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto shouldCenterOrigin = filterArgs.value<bool>(k_CenterOrigin_Key);
  auto shouldChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto origin = filterArgs.value<std::vector<float64>>(k_Origin_Key);
  auto spacing = filterArgs.value<std::vector<float64>>(k_Spacing_Key);

  std::optional<FloatVec3> optOrigin;
  std::optional<FloatVec3> optSpacing;

  FloatVec3 originVec = FloatVec3(origin[0], origin[1], origin[2]);
  FloatVec3 spacingVec = FloatVec3(spacing[0], spacing[1], spacing[2]);

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  bool centerOrigin = false;
  if(shouldChangeOrigin)
  {
    optOrigin = originVec;
    centerOrigin = shouldCenterOrigin;
  }
  if(shouldChangeSpacing)
  {
    optSpacing = spacingVec;
  }

  auto action = std::make_unique<UpdateImageGeomAction>(optOrigin, optSpacing, imageGeomPath, centerOrigin);

  resultOutputActions.value().appendAction(std::move(action));

  const auto* srcImageGeom = dataStructure.getDataAs<ImageGeom>(imageGeomPath);

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using the appropriate methods.
  preflightUpdatedValues.push_back({"Input Geometry  [Before Update]", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom->getDimensions(), srcImageGeom->getSpacing(),
                                                                                                                                    srcImageGeom->getOrigin(), srcImageGeom->getUnits())});

  preflightUpdatedValues.push_back(
      {"Image Geometry [After Update]", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom->getDimensions(), spacingVec, originVec, srcImageGeom->getUnits())});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> SetImageGeomOriginScalingFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_ChangeOriginKey = "ChangeOrigin";
constexpr StringLiteral k_OriginKey = "Origin";
constexpr StringLiteral k_ChangeResolutionKey = "ChangeResolution";
constexpr StringLiteral k_SpacingKey = "Spacing";
} // namespace SIMPL
} // namespace

Result<Arguments> SetImageGeomOriginScalingFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SetImageGeomOriginScalingFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ChangeOriginKey, k_ChangeOrigin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_OriginKey, k_Origin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ChangeResolutionKey, k_ChangeSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_SpacingKey, k_Spacing_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
