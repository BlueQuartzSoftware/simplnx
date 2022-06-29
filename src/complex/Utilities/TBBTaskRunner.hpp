#pragma once

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

// SIMPLib.h MUST be included before this or the guard will block the include but not its uses below.
// This is consistent with previous behavior, only earlier parallelization split the includes between
// the corresponding .h and .cpp files.
#ifdef COMPLEX_ENABLE_MULTICORE
#include <tbb/task_group.h>
#endif

namespace complex
{

/**
 * @brief TBBTaskRunner runs a functor in a tbb::task_group if multicore is enabled.
 * It does not spawn more tasks than there are threads. Once the max thread count
 * is reached, the task group waits until all current tasks have been completed.
 * If multicore is not enabled, functors run sequentially. wait() must be called
 * before the object destructs.
 */
class TBBTaskRunner
{
public:
  TBBTaskRunner()
  : m_MaxThreadCount(std::thread::hardware_concurrency()) // std::thread::hardware_concurrency() returns ZERO if not defined on this platform
  {
  }

  TBBTaskRunner(uint32 maxThreads)
  : m_MaxThreadCount(maxThreads)
  {
  }

  TBBTaskRunner(const TBBTaskRunner&) = delete;
  TBBTaskRunner(TBBTaskRunner&&) = delete;

  TBBTaskRunner& operator=(const TBBTaskRunner&) = delete;
  TBBTaskRunner& operator=(TBBTaskRunner&&) = delete;

  ~TBBTaskRunner() = default;

  auto wait()
  {
#ifdef COMPLEX_ENABLE_MULTICORE
    return m_TaskGroup.wait();
#endif
  }

  template <class FunctorT>
  void run(FunctorT&& func)
  {
#ifdef COMPLEX_ENABLE_MULTICORE
    if(m_CurrentThreadCount == m_MaxThreadCount)
    {
      m_TaskGroup.wait();
      m_CurrentThreadCount = 0;
    }
    m_TaskGroup.run(std::forward<FunctorT>(func));
    m_CurrentThreadCount++;
#else
    func();
#endif
  }

private:
#ifdef COMPLEX_ENABLE_MULTICORE
  tbb::task_group m_TaskGroup;
  uint32 m_CurrentThreadCount = 0;
  uint32 m_MaxThreadCount = 1;
#endif
};

} // namespace complex
