#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundaryCellsInputValues;

/**
 * @class ComputeBoundaryCellsDirect
 * @brief Counts boundary faces through direct store indexing.
 *
 * The sequential ZYX traversal reads a voxel and its valid face neighbors through
 * operator[]. It writes one output value through operator[]. This path is efficient
 * for resident stores but can cause repeated chunk access for a disk-backed input
 * or output. Cancellation returns success after completed slices.
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCellsDirect
{
public:
  /**
   * @brief Initializes direct boundary counting.
   * @param dataStructure Provides geometry, input, and output arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between slices.
   * @param inputValues Defines paths and counting policies.
   * @pre All arguments outlive this executor.
   */
  ComputeBoundaryCellsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCellsDirect() noexcept;

  ComputeBoundaryCellsDirect(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect(ComputeBoundaryCellsDirect&&) noexcept = delete;
  ComputeBoundaryCellsDirect& operator=(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect& operator=(ComputeBoundaryCellsDirect&&) noexcept = delete;

  /**
   * @brief Counts boundary faces for every voxel.
   * @return Success. Cancellation is a valid early return.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
