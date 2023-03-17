/**
miniIsosurface miniapp License Version 1.0
========================================================================
Copyright (c) 2017
National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
certain rights in this software.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may
   be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
========================================================================

Original Author: dbourge
Created: Feb 17, 2017
*/

#pragma once

#include "complex/Common/TypesUtility.hpp"
#include "complex/complex_export.hpp"

#include <array>
#include <vector>

namespace complex
{
class COMPLEX_EXPORT FlyingEdgesAlgorithm
{
  using cube = std::array<std::array<float32, 3>, 8>;
  using float32Cube = std::array<float32, 8>;

public:
  FlyingEdgesAlgorithm(util::Image3D const& image, float32 const& isoval)
  : m_Image(image)
  , m_IsoVal(isoval)
  , m_NX(image.xdimension())
  , m_NY(image.ydimension())
  , m_NZ(image.zdimension())
  , m_GridEdges(m_NY * m_NZ)
  , m_TriCounter((m_NY - 1) * (m_NZ - 1))
  , m_EdgeCases((m_NX - 1) * m_NY * m_NZ)
  , m_CubeCases((m_NX - 1) * (m_NY - 1) * (m_NZ - 1))
  {
  }

  void pass1();

  void pass2();

  void pass3();

  void pass4();

  util::TriangleMesh moveOutput();

private:
  struct gridEdge
  {
    gridEdge()
    : xl(0)
    , xr(0)
    , xstart(0)
    , ystart(0)
    , zstart(0)
    {
    }

    // trim values
    // set on pass 1
    usize xl;
    usize xr;

    // modified on pass 2
    // set on pass 3
    usize xstart;
    usize ystart;
    usize zstart;
  };

  ///////////////////// MEMBER VARIABLES /////////////////////

  util::Image3D const& m_Image;
  float32 const m_IsoVal;

  usize const m_NX; //
  usize const m_NY; // for indexing
  usize const m_NZ; //

  std::vector<gridEdge> m_GridEdges; // size of ny*nz
  std::vector<usize> m_TriCounter;  // size of (ny-1)*(nz-1)

  std::vector<uint8> m_EdgeCases; // size (nx-1)*ny*nz
  std::vector<uint8> m_CubeCases; // size (nx-1)*(ny-1)*(nz-1)

  std::vector<std::array<float32, 3>> m_Points;  //
  std::vector<std::array<float32, 3>> m_Normals; // The output
  std::vector<std::array<usize, 3>> m_Tris;      //

  /////////////////////////////////////////////////////////////

  bool isCutEdge(usize const& i, usize const& j, usize const& k) const;

  inline uint8 calcCaseEdge(bool const& prevEdge, bool const& currEdge) const;

  inline uint8 calcCubeCase(uint8 const& ec0, uint8 const& ec1, uint8 const& ec2, uint8 const& ec3) const;

  inline void calcTrimValues(usize& xl, usize& xr, usize const& j, usize const& k) const;

  inline std::array<float32, 3> interpolateOnCube(cube const& pts, float32Cube const& isovals, uint8 const& edge) const;

  inline std::array<float32, 3> interpolate(std::array<float32, 3> const& a, std::array<float32, 3> const& b, float32 const& weight) const;
};
} // namespace complex