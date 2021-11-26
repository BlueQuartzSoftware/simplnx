/* ============================================================================
 * Copyright (c) 2019 BlueQuartz Software, LLC
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
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
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

#include "complex/Common/ComplexRange.hpp"

#include <stdexcept>
using namespace complex;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange::ComplexRange()
: m_Range({{0, 0}})
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange::ComplexRange(size_t begin, size_t end)
: m_Range({{begin, end}})
{
}

#ifdef COMPLEX_ENABLE_MULTICORE
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange::ComplexRange(const tbb::blocked_range<size_t>& r)
: m_Range({{r.begin(), r.end()}})
{
}
#endif

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ComplexRange::RangeType ComplexRange::getRange() const
{
  return m_Range;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange::min() const
{
  return m_Range[0];
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange::max() const
{
  return m_Range[1];
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange::size() const
{
  return m_Range[1] - m_Range[0];
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool ComplexRange::empty() const
{
  return size() == 0;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ComplexRange::operator[](size_t index) const
{
  switch(index)
  {
  case 0:
    return min();
  case 1:
    return max();
  default:
    throw std::range_error("Range must be 0 or 1");
  }
}
