#include "ParallelData2DAlgorithm.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ParallelData2DAlgorithm::ParallelData2DAlgorithm() = default;

// -----------------------------------------------------------------------------
ParallelData2DAlgorithm::~ParallelData2DAlgorithm() = default;

// -----------------------------------------------------------------------------
bool ParallelData2DAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
void ParallelData2DAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_RunParallel = doParallel;
}

// -----------------------------------------------------------------------------
Range2D ParallelData2DAlgorithm::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
void ParallelData2DAlgorithm::setRange(const RangeType& range)
{
  m_Range = range;
}

// -----------------------------------------------------------------------------
void ParallelData2DAlgorithm::setRange(size_t minCols, size_t maxCols, size_t minRows, size_t maxRows)
{
  m_Range = {minCols, maxCols, minRows, maxRows};
}

// -----------------------------------------------------------------------------
std::optional<Range2D> ParallelData2DAlgorithm::getChunkSize() const
{
  return m_ChunkSize;
}

// -----------------------------------------------------------------------------
void ParallelData2DAlgorithm::setChunkSize(std::optional<RangeType> chunkSize)
{
  m_ChunkSize = chunkSize;
}
