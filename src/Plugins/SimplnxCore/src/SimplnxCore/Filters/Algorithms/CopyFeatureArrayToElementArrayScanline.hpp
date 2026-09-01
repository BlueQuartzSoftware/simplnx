#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct CopyFeatureArrayToElementArrayInputValues;

/**
 * @class CopyFeatureArrayToElementArrayScanline
 * @brief Broadcasts feature tuples through bounded bulk I/O.
 *
 * The algorithm caches one feature-scale source and streams 65,536 cell tuples.
 * This avoids per-cell disk access and partial-tuple writes. The feature cache can
 * be large when the feature count approaches the cell count.
 *
 * @see CopyFeatureArrayToElementArrayDirect for the in-core variant.
 * @see CopyFeatureArrayToElementArray for the dispatcher.
 */
class SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArrayScanline
{
public:
  /**
   * @brief Creates a bulk-I/O feature broadcast algorithm.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later chunks when true.
   * @param inputValues Specifies validated paths and naming. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  CopyFeatureArrayToElementArrayScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         const CopyFeatureArrayToElementArrayInputValues* inputValues);
  /**
   * @brief Destroys the non-owning bulk-I/O algorithm.
   */
  ~CopyFeatureArrayToElementArrayScanline() noexcept;

  CopyFeatureArrayToElementArrayScanline(const CopyFeatureArrayToElementArrayScanline&) = delete;
  CopyFeatureArrayToElementArrayScanline(CopyFeatureArrayToElementArrayScanline&&) noexcept = delete;
  CopyFeatureArrayToElementArrayScanline& operator=(const CopyFeatureArrayToElementArrayScanline&) = delete;
  CopyFeatureArrayToElementArrayScanline& operator=(CopyFeatureArrayToElementArrayScanline&&) noexcept = delete;

  /**
   * @brief Broadcasts every selected feature array.
   * @return Error from validation or bulk I/O, or success after cancellation.
   *
   * Cancellation after a Feature Id read does not write that chunk.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CopyFeatureArrayToElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
