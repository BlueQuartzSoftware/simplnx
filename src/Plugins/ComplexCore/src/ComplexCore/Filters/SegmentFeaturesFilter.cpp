#include "SegmentFeaturesFilter.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

namespace complex
{
namespace
{
inline constexpr int32 k_MissingGeomError = -440;
}

std::string SegmentFeaturesFilter::name() const
{
  return FilterTraits<SegmentFeaturesFilter>::name;
}

std::string SegmentFeaturesFilter::className() const
{
  return FilterTraits<SegmentFeaturesFilter>::className;
}

Uuid SegmentFeaturesFilter::uuid() const
{
  return FilterTraits<SegmentFeaturesFilter>::uuid;
}

std::string SegmentFeaturesFilter::humanName() const
{
  return "Segment Features";
}

Parameters SegmentFeaturesFilter::parameters() const
{
  Parameters params;
  return params;
}

IFilter::UniquePointer SegmentFeaturesFilter::clone() const
{
  return std::make_unique<SegmentFeaturesFilter>();
}

IFilter::PreflightResult SegmentFeaturesFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{

  OutputActions actions;
  return {std::move(actions)};
}

Result<> SegmentFeaturesFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  return {};
}

} // namespace complex
