#include "ParallelDataAlgorithm.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ParallelDataAlgorithm::ParallelDataAlgorithm()
: m_Range(ComplexRange())
#ifdef COMPLEX_ENABLE_MULTICORE
, m_RunParallel(true)
, m_Partitioner(tbb::auto_partitioner())
#endif
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ParallelDataAlgorithm::~ParallelDataAlgorithm() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool ParallelDataAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelDataAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_RunParallel = doParallel;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange ParallelDataAlgorithm::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelDataAlgorithm::setRange(const ComplexRange& range)
{
  m_Range = range;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelDataAlgorithm::setRange(size_t min, size_t max)
{
  m_Range = {min, max};
}

#ifdef COMPLEX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelDataAlgorithm::setPartitioner(const tbb::auto_partitioner& partitioner)
{
  m_Partitioner = partitioner;
}
#endif
