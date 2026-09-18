#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

/**
 * @struct ConditionalSetValueInputValues
 * @brief Stores validated replacement paths and options.
 */
struct SIMPLNXCORE_EXPORT ConditionalSetValueInputValues
{
  ArraySelectionParameter::ValueType ConditionalArrayPath;
  BoolParameter::ValueType InvertMask;
  StringParameter::ValueType RemoveValue;
  StringParameter::ValueType ReplaceValue;
  ArraySelectionParameter::ValueType SelectedArrayPath;
  BoolParameter::ValueType UseConditional;
};

/**
 * @class ConditionalSetValue
 * @brief Replaces selected array values using either value comparison or a conditional mask.
 *
 * Conditional replacement changes every component of a selected tuple. The
 * Scanline path keeps complete tuples in bounded buffers to avoid partial-tuple
 * read-modify-write operations. A false invert flag selects nonzero conditions.
 * A true invert flag selects zero conditions.
 */
class SIMPLNXCORE_EXPORT ConditionalSetValue
{
public:
  /**
   * @brief Creates a conditional replacement dispatcher.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later chunks when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  ConditionalSetValue(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConditionalSetValueInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~ConditionalSetValue() noexcept;

  ConditionalSetValue(const ConditionalSetValue&) = delete;
  ConditionalSetValue(ConditionalSetValue&&) noexcept = delete;
  ConditionalSetValue& operator=(const ConditionalSetValue&) = delete;
  ConditionalSetValue& operator=(ConditionalSetValue&&) noexcept = delete;

  /**
   * @brief Replaces selected values.
   * @return Error from conversion or bulk I/O, or success after cancellation.
   *
   * Cancellation can retain replacements from completed chunks.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ConditionalSetValueInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
