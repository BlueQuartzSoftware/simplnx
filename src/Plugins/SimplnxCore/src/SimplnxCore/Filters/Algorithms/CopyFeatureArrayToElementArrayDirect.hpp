#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct CopyFeatureArrayToElementArrayInputValues;

/**
 * @class CopyFeatureArrayToElementArrayDirect
 * @brief Broadcasts feature tuples through in-memory arrays.
 *
 * Concrete DataStore instances use raw pointers for parallel ranges. The generic
 * fallback accesses DataStore instances in parallel and has no general
 * thread-safety guarantee. Scanline avoids disk-backed per-cell lookups.
 *
 * @see CopyFeatureArrayToElementArrayScanline for the OOC-optimized variant.
 * @see CopyFeatureArrayToElementArray for the dispatcher.
 */
class SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArrayDirect
{
public:
  /**
   * @brief Creates an in-memory feature broadcast algorithm.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later ranges when true.
   * @param inputValues Specifies validated paths and naming. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  CopyFeatureArrayToElementArrayDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                       const CopyFeatureArrayToElementArrayInputValues* inputValues);
  /**
   * @brief Destroys the non-owning in-memory algorithm.
   */
  ~CopyFeatureArrayToElementArrayDirect() noexcept;

  CopyFeatureArrayToElementArrayDirect(const CopyFeatureArrayToElementArrayDirect&) = delete;
  CopyFeatureArrayToElementArrayDirect(CopyFeatureArrayToElementArrayDirect&&) noexcept = delete;
  CopyFeatureArrayToElementArrayDirect& operator=(const CopyFeatureArrayToElementArrayDirect&) = delete;
  CopyFeatureArrayToElementArrayDirect& operator=(CopyFeatureArrayToElementArrayDirect&&) noexcept = delete;

  /**
   * @brief Broadcasts every selected feature array.
   * @return Error from Feature Id validation, or success after cancellation.
   *
   * Cancellation can retain output from completed parallel ranges.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CopyFeatureArrayToElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
