/* ============================================================================
 * Copyright (c) 2019 BlueQuartz Software, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the names of any of the BlueQuartz Software contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The code contained herein was partially funded by the following contracts:
 *    United States Air Force Prime Contract FA8650-15-D-5231
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ParallelData3DAlgorithm.hpp"
using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::ParallelData3DAlgorithm()
: m_Range(ComplexRange3D())
#ifdef COMPLEX_ENABLE_MULTICORE
, m_RunParallel(true)
, m_Partitioner(tbb::auto_partitioner())
#endif
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ParallelData3DAlgorithm::~ParallelData3DAlgorithm() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool ParallelData3DAlgorithm::getParallelizationEnabled() const
{
  return m_RunParallel;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_RunParallel = doParallel;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange3D ParallelData3DAlgorithm::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setRange(const ComplexRange3D& range)
{
  m_Range = range;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setRange(size_t xMax, size_t yMax, size_t zMax)
{
  m_Range = {0, xMax, 0, yMax, 0, zMax};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ParallelData3DAlgorithm::getGrain() const
{
  return m_Grain;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setGrain(size_t grain)
{
  m_Grain = grain;
}

#ifdef COMPLEX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelData3DAlgorithm::setPartitioner(const tbb::auto_partitioner& partitioner)
{
  m_Partitioner = partitioner;
}
#endif
