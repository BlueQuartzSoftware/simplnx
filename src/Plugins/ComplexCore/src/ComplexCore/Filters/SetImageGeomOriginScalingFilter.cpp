#include "SetImageGeomOriginScalingFilter.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/UpdateImageGeomAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

#include <fmt/format.h>

#include <optional>

namespace complex
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

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "Path to the target ImageGeom", DataPath(), std::set{IGeometry::Type::Image}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Set Origin", "Specifies if the origin should be changed", true));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Origin_Key, "Origin  (Physical Units)", "Specifies the new origin values in physical units.", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeResolution_Key, "Set Spacing", "Specifies if the spacing should be changed", true));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing (Physical Units)", "Specifies the new spacing values in physical units.", std::vector<float64>{1, 1, 1},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeResolution_Key, k_Spacing_Key, std::make_any<bool>(true));
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
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto shouldChangeResolution = filterArgs.value<bool>(k_ChangeResolution_Key);
  auto origin = filterArgs.value<std::vector<float64>>(k_Origin_Key);
  auto spacing = filterArgs.value<std::vector<float64>>(k_Spacing_Key);

  std::optional<FloatVec3> optOrigin;
  std::optional<FloatVec3> optSpacing;

  FloatVec3 originVec = FloatVec3(origin[0], origin[1], origin[2]);
  FloatVec3 spacingVec = FloatVec3(spacing[0], spacing[1], spacing[2]);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(shouldChangeOrigin)
  {
    optOrigin = originVec;
  }
  if(shouldChangeResolution)
  {
    spacingVec = spacingVec;
  }

  auto action = std::make_unique<UpdateImageGeomAction>(optOrigin, spacingVec, imageGeomPath);

  resultOutputActions.value().appendAction(std::move(action));

  const auto* srcImageGeom = dataStructure.getDataAs<ImageGeom>(imageGeomPath);

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using the appropriate methods.
  preflightUpdatedValues.push_back({"Input Geometry  [Before Update]", complex::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom->getDimensions(), srcImageGeom->getSpacing(),
                                                                                                                                   srcImageGeom->getOrigin(), srcImageGeom->getUnits())});

  preflightUpdatedValues.push_back(
      {"Image Geometry [After Update]", complex::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom->getDimensions(), spacingVec, originVec, srcImageGeom->getUnits())});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> SetImageGeomOriginScalingFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
