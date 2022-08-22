#include "ParallelData3DAlgorithm.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::ParallelData3DAlgorithm()
: m_Range(Range3D())
#ifdef COMPLEX_ENABLE_MULTICORE
, m_RunParallel(true)
, m_Partitioner(tbb::auto_partitioner())
#endif
{
}

// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::~ParallelData3DAlgorithm() = default;

// -----------------------------------------------------------------------------
bool ParallelData3DAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_RunParallel = doParallel;
}

// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::RangeType ParallelData3DAlgorithm::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setRange(const RangeType& range)
{
  m_Range = range;
}

// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setRange(size_t xMax, size_t yMax, size_t zMax)
{
  m_Range = {0, xMax, 0, yMax, 0, zMax};
}

#ifdef COMPLEX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setPartitioner(const tbb::auto_partitioner& partitioner)
{
  m_Partitioner = partitioner;
}
#endif
