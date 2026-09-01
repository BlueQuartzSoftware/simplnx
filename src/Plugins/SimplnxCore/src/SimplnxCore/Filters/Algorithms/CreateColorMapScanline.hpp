#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct CreateColorMapInputValues;

/**
 * @class CreateColorMapScanline
 * @brief Maps scalar tuples to RGB values through bounded bulk I/O.
 *
 * The first pass finds the exact typed range. The second pass maps 65,536 tuples
 * at a time and bulk-writes RGB values. Memory remains independent of tuple count.
 */
class SIMPLNXCORE_EXPORT CreateColorMapScanline
{
public:
  /**
   * @brief Creates a bulk-I/O color-map algorithm.
   * @param dataStructure Provides selected arrays.
   * @param msgHandler Receives progress messages.
   * @param shouldCancel Stops before later chunks when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  CreateColorMapScanline(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const CreateColorMapInputValues* inputValues);
  /**
   * @brief Destroys the non-owning bulk-I/O algorithm.
   */
  ~CreateColorMapScanline() noexcept;

  CreateColorMapScanline(const CreateColorMapScanline&) = delete;
  CreateColorMapScanline(CreateColorMapScanline&&) noexcept = delete;
  CreateColorMapScanline& operator=(const CreateColorMapScanline&) = delete;
  CreateColorMapScanline& operator=(CreateColorMapScanline&&) noexcept = delete;

  /**
   * @brief Maps scalar tuples to RGB values.
   * @return Preset or bulk-I/O error, or success after cancellation.
   *
   * Cancellation during mapping retains completed RGB chunks. An unsupported
   * mask returns success without writing colors.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateColorMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
