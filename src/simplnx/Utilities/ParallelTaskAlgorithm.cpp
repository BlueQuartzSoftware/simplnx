#include "ParallelTaskAlgorithm.hpp"

#include <algorithm>
#include <thread>

using namespace nx::core;

// -----------------------------------------------------------------------------
ParallelTaskAlgorithm::ParallelTaskAlgorithm() = default;

// -----------------------------------------------------------------------------
ParallelTaskAlgorithm::~ParallelTaskAlgorithm()
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  m_TaskGroup.wait();
#endif
}

// -----------------------------------------------------------------------------
uint32_t ParallelTaskAlgorithm::getMaxThreads() const
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  return m_MaxThreads;
#else
  return std::thread::hardware_concurrency();
#endif
}

// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::setMaxThreads(uint32_t threads)
{
  m_MaxThreads = std::min(threads, std::thread::hardware_concurrency());
}

// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::wait()
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  // This will spill over if the number of files to process does not divide evenly by the number of threads.
  m_TaskGroup.wait();
  m_CurThreads = 0;
#endif
}
