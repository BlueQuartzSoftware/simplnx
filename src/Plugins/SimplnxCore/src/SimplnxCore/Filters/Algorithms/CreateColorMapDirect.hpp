#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct CreateColorMapInputValues;

/**
 * @class CreateColorMapDirect
 * @brief Maps scalar tuples to RGB values through in-memory arrays.
 *
 * Concrete stores use raw pointers for parallel mapping. The abstract fallback
 * accesses DataStore instances in parallel and has no general thread-safety
 * guarantee. Disk-backed arrays dispatch to Scanline.
 */
class SIMPLNXCORE_EXPORT CreateColorMapDirect
{
public:
  /**
   * @brief Creates an in-memory color-map algorithm.
   * @param dataStructure Provides selected arrays.
   * @param msgHandler Receives progress messages.
   * @param shouldCancel Is retained but not checked by this direct path.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  CreateColorMapDirect(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const CreateColorMapInputValues* inputValues);
  /**
   * @brief Destroys the non-owning in-memory algorithm.
   */
  ~CreateColorMapDirect() noexcept;

  CreateColorMapDirect(const CreateColorMapDirect&) = delete;
  CreateColorMapDirect(CreateColorMapDirect&&) noexcept = delete;
  CreateColorMapDirect& operator=(const CreateColorMapDirect&) = delete;
  CreateColorMapDirect& operator=(CreateColorMapDirect&&) noexcept = delete;

  /**
   * @brief Maps scalar tuples to RGB values.
   * @return Preset error, or success.
   *
   * The current caller discards typed-generator errors, including empty inputs.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateColorMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
