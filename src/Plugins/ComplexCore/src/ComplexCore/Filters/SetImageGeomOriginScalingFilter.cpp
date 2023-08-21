#include "SetImageGeomOriginScalingFilter.hpp"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/UpdateImageGeomAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

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
  return {className(), "Image Geometry", "Shift", "Spacing", "Mutate"};
}

//------------------------------------------------------------------------------
Parameters SetImageGeomOriginScalingFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "Path to the target ImageGeom", DataPath(), std::set{IGeometry::Type::Image}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "Specifies if the origin should be changed", true));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Origin_Key, "Origin", "Specifies the new origin values", std::vector<float64>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeResolution_Key, "Change Spacing", "Specifies if the spacing should be changed", true));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing", "Specifies the new spacing values", std::vector<float64>{1, 1, 1}, std::vector<std::string>{"X", "Y", "Z"}));

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

  std::optional<FloatVec3> originVec;
  std::optional<FloatVec3> spacingVec;

  if(shouldChangeOrigin)
  {
    originVec = FloatVec3(origin[0], origin[1], origin[2]);
  }
  if(shouldChangeResolution)
  {
    spacingVec = FloatVec3(spacing[0], spacing[1], spacing[2]);
  }

  auto action = std::make_unique<UpdateImageGeomAction>(originVec, spacingVec, imageGeomPath);

  OutputActions actions;
  actions.appendAction(std::move(action));
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> SetImageGeomOriginScalingFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
