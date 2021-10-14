#include "AlignGeometries.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string AlignGeometries::name() const
{
  return FilterTraits<AlignGeometries>::name.str();
}

Uuid AlignGeometries::uuid() const
{
  return FilterTraits<AlignGeometries>::uuid;
}

std::string AlignGeometries::humanName() const
{
  return "Align Geometries";
}

Parameters AlignGeometries::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_MovingGeometry_Key, "Moving Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TargetGeometry_Key, "Target Geometry", "", DataPath{}));
  params.insert(std::make_unique<ChoicesParameter>(k_AlignmentType_Key, "Alignment Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));

  return params;
}

IFilter::UniquePointer AlignGeometries::clone() const
{
  return std::make_unique<AlignGeometries>();
}

Result<OutputActions> AlignGeometries::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMovingGeometryValue = filterArgs.value<DataPath>(k_MovingGeometry_Key);
  auto pTargetGeometryValue = filterArgs.value<DataPath>(k_TargetGeometry_Key);
  auto pAlignmentTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_AlignmentType_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<AlignGeometriesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> AlignGeometries::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMovingGeometryValue = filterArgs.value<DataPath>(k_MovingGeometry_Key);
  auto pTargetGeometryValue = filterArgs.value<DataPath>(k_TargetGeometry_Key);
  auto pAlignmentTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_AlignmentType_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
