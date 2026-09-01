#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeVectorColorsInputValues
 * @brief Stores validated vector, mask, and color-output paths.
 */
struct SIMPLNXCORE_EXPORT ComputeVectorColorsInputValues
{
  bool UseMask;
  DataPath VectorsArrayPath;
  DataPath MaskArrayPath;
  DataPath CellVectorColorsArrayPath;
};

/**
 * @class ComputeVectorColors
 * @brief Converts vector directions to RGB colors using bounded chunk buffers.
 *
 * The algorithm bulk-reads 65,536 vector tuples per chunk and bulk-writes RGB
 * tuples. This preserves color mapping without per-tuple disk access. A zero
 * mask value leaves the corresponding RGB tuple black.
 */
class SIMPLNXCORE_EXPORT ComputeVectorColors
{
public:
  /**
   * @brief Creates a vector-color algorithm.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later chunks when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  ComputeVectorColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeVectorColorsInputValues* inputValues);

  /**
   * @brief Destroys the non-owning vector-color algorithm.
   */
  ~ComputeVectorColors() noexcept;

  ComputeVectorColors(const ComputeVectorColors&) = delete;
  ComputeVectorColors(ComputeVectorColors&&) noexcept = delete;
  ComputeVectorColors& operator=(const ComputeVectorColors&) = delete;
  ComputeVectorColors& operator=(ComputeVectorColors&&) noexcept = delete;

  /**
   * @brief Converts vector tuples to RGB colors.
   * @return Error from bulk I/O or mask validation, or success after cancellation.
   *
   * Cancellation retains colors from completed chunks.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeVectorColorsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
