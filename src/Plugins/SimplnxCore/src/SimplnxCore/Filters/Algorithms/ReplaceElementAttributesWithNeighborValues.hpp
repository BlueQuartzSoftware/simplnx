#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{
namespace detail
{
static inline constexpr StringLiteral k_LessThan = "< [Less Than]";
static inline constexpr StringLiteral k_GreaterThan = "> [Greater Than]";
static inline const ChoicesParameter::Choices k_OperationChoices = {k_LessThan, k_GreaterThan};
} // namespace detail

struct SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValuesInputValues
{
  float32 MinConfidence;
  ChoicesParameter::ValueType SelectedComparison;
  bool Loop;
  DataPath InputArrayPath;
  DataPath SelectedImageGeometryPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValues
{
public:
  ReplaceElementAttributesWithNeighborValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ReplaceElementAttributesWithNeighborValuesInputValues* inputValues);
  ~ReplaceElementAttributesWithNeighborValues() noexcept;

  ReplaceElementAttributesWithNeighborValues(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReplaceElementAttributesWithNeighborValuesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
