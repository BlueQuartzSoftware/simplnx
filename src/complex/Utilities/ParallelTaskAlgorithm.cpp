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

#include "ParallelTaskAlgorithm.hpp"

#include <algorithm>
#include <thread>

using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ParallelTaskAlgorithm::ParallelTaskAlgorithm()
: m_Parallelization(true)
, m_MaxThreads(std::thread::hardware_concurrency())
#ifdef COMPLEX_ENABLE_MULTICORE
, m_TaskGroup(new tbb::task_group)
#endif
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ParallelTaskAlgorithm::~ParallelTaskAlgorithm()
{
#ifdef COMPLEX_ENABLE_MULTICORE
  m_TaskGroup->wait();
#endif
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool ParallelTaskAlgorithm::getParallelizationEnabled() const
{
  return m_Parallelization;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::setParallelizationEnabled(bool doParallel)
{
  m_Parallelization = doParallel;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
uint32_t ParallelTaskAlgorithm::getMaxThreads() const
{
#ifdef COMPLEX_ENABLE_MULTICORE
  return m_MaxThreads;
#else
  return std::thread::hardware_concurrency();
#endif
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::setMaxThreads(uint32_t threads)
{
  m_MaxThreads = std::min(threads, std::thread::hardware_concurrency());
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ParallelTaskAlgorithm::wait()
{
#ifdef COMPLEX_ENABLE_MULTICORE
  // This will spill over if the number of files to process does not divide evenly by the number of threads.
  m_TaskGroup->wait();
  m_CurThreads = 0;
#endif
}
