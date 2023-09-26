#pragma once

#include "complex/Utilities/IParallelAlgorithm.hpp"
#include "complex/complex_export.hpp"

#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/task_group.h>
#endif

#include <array>
#include <cstddef>
#include <memory>

namespace complex
{

/**
 * @brief The ParallelTaskAlgorithm class handles parallelization across task-based algorithms.
 * An object with a function operator is required to operate the task.  This class utilizes
 * TBB for parallelization and will fallback to non-parallelization if it is not available
 * or the parallelization is disabled.
 */
class COMPLEX_EXPORT ParallelTaskAlgorithm : public IParallelAlgorithm
{
public:
  ParallelTaskAlgorithm();
  virtual ~ParallelTaskAlgorithm();

  /**
   * @brief Return maximum threads to use for parallelization.  If Parallel Algorithms
   * is not enabled, the maximum hardware concurrency is returned instead.
   * @return
   */
  uint32_t getMaxThreads() const;

  /**
   * @brief Sets the maximum number of threads to use.  This amount is automatically
   * reduced to the max hardware concurrency.
   * @param threads
   */
  void setMaxThreads(uint32_t threads);

  /**
   * @brief Executes the given object's function operator.  If parallel algorithms
   * is enabled, this process is multi-threaded.  Otherwise, this process is done
   * in a single thread.
   * @param body
   */
  template <typename Body>
  void execute(const Body& body)
  {
#ifdef COMPLEX_ENABLE_MULTICORE
    if(getParallelizationEnabled())
    {
      m_TaskGroup.run(body);
      m_CurThreads++;
      if(m_CurThreads >= m_MaxThreads)
      {
        wait();
      }
    }
    else
#endif
    {
      body();
    }
  }

  /**
   * @brief Waits for the threads to finish and resets the current thread count.
   */
  void wait();

private:
#ifdef COMPLEX_ENABLE_MULTICORE
  uint32_t m_MaxThreads = std::thread::hardware_concurrency();
  tbb::task_group m_TaskGroup;
  uint32_t m_CurThreads = 0;
#else
  uint32_t m_MaxThreads = 1;
#endif
};
} // namespace complex
