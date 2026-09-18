#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"

#include <nonstd/span.hpp>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#endif

#include <algorithm>
#include <exception>
#include <functional>
#include <mutex>
#include <utility>

namespace nx::core::HDF5
{
/**
 * @brief Runs one body invocation for each local chunk position.
 * @tparam Body Specifies a callable that accepts one usize position.
 * @param chunkCount Specifies the number of positions.
 * @param body Processes one position.
 * @pre body supports concurrent invocations when multicore support is enabled.
 *
 * Multicore builds use the process-wide oneTBB scheduler. Other builds use the
 * calling thread. Each invocation receives a different position. Exceptions use
 * the active scheduler's propagation behavior.
 */
template <class Body>
void ParallelForChunkPositions(usize chunkCount, const Body& body)
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  if(chunkCount > 1)
  {
    tbb::static_partitioner partitioner;
    tbb::parallel_for(
        tbb::blocked_range<usize>(0, chunkCount, 1),
        [&body](const tbb::blocked_range<usize>& range) {
          for(usize localIndex = range.begin(); localIndex < range.end(); ++localIndex)
          {
            body(localIndex);
          }
        },
        partitioner);
    return;
  }
#endif
  for(usize localIndex = 0; localIndex < chunkCount; ++localIndex)
  {
    body(localIndex);
  }
}

/**
 * @brief Loads and consumes each selected chunk position.
 * @tparam Result Specifies the moveable value that loader produces and sink consumes.
 * @param chunkIndices Supplies one chunk index for each local position. An empty span is a no-op.
 * @param loader Produces a result for one chunk index.
 * @param sink Consumes one local position and its result.
 * @throws std::exception Rethrows the first observed non-skip exception after scheduled work finishes.
 * @pre loader and sink support concurrent invocations when multicore support is enabled.
 *
 * Each task owns a different local position. A sink can write to that position in
 * a pre-sized container without a lock. This property does not make generic
 * DataStore, AbstractDataStore, or shared external state thread-safe.
 *
 * The callable seam lets callers keep decompression outside the process-wide HDF5
 * lock. Deflate work is CPU-intensive and can run on worker threads. The seam also
 * supports move-only results and different sinks with one thread harness.
 *
 * UnallocatedChunkError skips sink output for that position. The exception can
 * mean sparse storage or unavailable raw metadata. This function cannot identify
 * the cause. The catch covers both callables, so sink must not throw this exception
 * unless skip behavior is correct. Other exceptions are recorded under a mutex.
 * Remaining scheduled positions continue. Concurrent execution makes the first
 * observed exception order nondeterministic.
 */
template <class Result>
void ParallelLoadChunks(nonstd::span<const uint64> chunkIndices, const std::function<Result(uint64 chunkIndex)>& loader, const std::function<void(usize localIndex, Result&&)>& sink)
{
  const usize chunkCount = chunkIndices.size();
  if(chunkCount == 0)
  {
    return;
  }

  std::mutex errMutex;
  std::exception_ptr firstError = nullptr;

  const auto loadOne = [&](usize i) {
    try
    {
      Result result = loader(chunkIndices[i]);
      // A sink can write one pre-sized local slot without sharing that slot.
      sink(i, std::move(result));
    } catch(const UnallocatedChunkError&)
    {
      // Raw bytes are unavailable. Leave the caller's local slot unchanged.
      // The exception does not distinguish sparse storage from metadata failure.
      return;
    } catch(...)
    {
      // Record the first observed failure. Other scheduled positions still run.
      std::lock_guard<std::mutex> lk(errMutex);
      if(!firstError)
      {
        firstError = std::current_exception();
      }
    }
  };

  ParallelForChunkPositions(chunkCount, loadOne);

  if(firstError)
  {
    std::rethrow_exception(firstError);
  }
}
} // namespace nx::core::HDF5
