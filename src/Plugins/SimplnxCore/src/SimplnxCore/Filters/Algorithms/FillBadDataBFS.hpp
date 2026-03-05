#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct FillBadDataInputValues;

/**
 * @class FillBadDataBFS
 * @brief BFS flood-fill algorithm for filling bad data regions.
 *
 * This is the in-core-optimized implementation. It uses BFS (breadth-first search)
 * to identify connected components of bad data, then iteratively fills small regions
 * by voting among face neighbors. Uses O(N) temporary buffers (neighbors, alreadyChecked)
 * which is efficient when data fits in RAM.
 *
 * @see FillBadDataCCL for the out-of-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT FillBadDataBFS
{
public:
  /**
   * @brief Constructs the BFS fill algorithm with the required context.
   * @param dataStructure The data structure containing the arrays to process.
   * @param mesgHandler Handler for progress and informational messages.
   * @param shouldCancel Cancellation flag checked during execution.
   * @param inputValues Filter parameter values controlling fill behavior.
   */
  FillBadDataBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadDataBFS() noexcept;

  FillBadDataBFS(const FillBadDataBFS&) = delete;
  FillBadDataBFS(FillBadDataBFS&&) noexcept = delete;
  FillBadDataBFS& operator=(const FillBadDataBFS&) = delete;
  FillBadDataBFS& operator=(FillBadDataBFS&&) noexcept = delete;

  /**
   * @brief Executes the BFS flood-fill algorithm to identify and fill bad data regions.
   * @return Result indicating success or an error with a descriptive message.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
