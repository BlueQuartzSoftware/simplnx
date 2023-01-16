#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace
{
const std::string k_LessThan("< [Less Than]");
const std::string k_GreaterThan("> [Greater Than]");
const complex::ChoicesParameter::Choices k_OperationChoices = {k_LessThan, k_GreaterThan};
} // namespace

namespace complex
{

struct COMPLEXCORE_EXPORT ReplaceElementAttributesWithNeighborValuesInputValues
{
  float32 MinConfidence;
  ChoicesParameter::ValueType SelectedComparison;
  bool Loop;
  DataPath InputArrayPath;
  DataPath SelectedImageGeometryPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ReplaceElementAttributesWithNeighborValues
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

} // namespace complex
