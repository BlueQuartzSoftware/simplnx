#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct IdentifySampleInputValues;

/**
 * @class IdentifySampleBFS
 * @brief BFS flood-fill algorithm for identifying the largest sample region.
 *
 * This is the in-core-optimized implementation. It uses BFS (breadth-first search)
 * with std::vector<bool> for tracking visited voxels, which is memory-efficient
 * (1 bit per voxel) and fast when data is in contiguous memory. However, the random
 * access pattern of BFS causes severe chunk thrashing in out-of-core mode.
 *
 * @see IdentifySampleCCL for the out-of-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT IdentifySampleBFS
{
public:
  /**
   * @brief Constructs the BFS sample identification algorithm with the required context.
   * @param dataStructure The data structure containing the arrays to process.
   * @param mesgHandler Handler for progress and informational messages.
   * @param shouldCancel Cancellation flag checked during execution.
   * @param inputValues Filter parameter values controlling identification behavior.
   */
  IdentifySampleBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues);
  ~IdentifySampleBFS() noexcept;

  IdentifySampleBFS(const IdentifySampleBFS&) = delete;
  IdentifySampleBFS(IdentifySampleBFS&&) noexcept = delete;
  IdentifySampleBFS& operator=(const IdentifySampleBFS&) = delete;
  IdentifySampleBFS& operator=(IdentifySampleBFS&&) noexcept = delete;

  /**
   * @brief Executes the BFS flood-fill algorithm to identify the largest sample region.
   * @return Result indicating success or an error with a descriptive message.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IdentifySampleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
