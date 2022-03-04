#include "SetOriginResolutionImageGeom.hpp"

#include <string>
#include <optional>

#include "fmt/format.h"

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/UpdateImageGeomAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

namespace
{
constexpr complex::int32 k_EMPTY_PARAMETER = -123;
} // namespace

namespace complex
{

std::string SetOriginResolutionImageGeom::name() const
{
  return FilterTraits<SetOriginResolutionImageGeom>::name;
}

std::string SetOriginResolutionImageGeom::className() const
{
  return FilterTraits<SetOriginResolutionImageGeom>::className;
}

Uuid SetOriginResolutionImageGeom::uuid() const
{
  return FilterTraits<SetOriginResolutionImageGeom>::uuid;
}

std::string SetOriginResolutionImageGeom::humanName() const
{
  return "Set Origin Resolution Image Geom";
}

Parameters SetOriginResolutionImageGeom::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "Path to the target ImageGeom", DataPath(), std::set{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "Specifies if the origin should be changed", true));
  params.insert(std::make_unique<BoolParameter>(k_ChangeResolution_Key, "Change Resolution", "Specifies if the resolution should be changed", true));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Origin_Key, "Origin", "Specifies the new origin values", std::vector<float64>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing", "Specifies the new spacing values", std::vector<float64>{1, 1, 1}, std::vector<std::string>{"X", "Y", "Z"}));
  return params;
}

IFilter::UniquePointer SetOriginResolutionImageGeom::clone() const
{
  return std::make_unique<SetOriginResolutionImageGeom>();
}

IFilter::PreflightResult SetOriginResolutionImageGeom::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  actions.actions.push_back(std::move(action));
  return {std::move(actions)};
}

Result<> SetOriginResolutionImageGeom::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
