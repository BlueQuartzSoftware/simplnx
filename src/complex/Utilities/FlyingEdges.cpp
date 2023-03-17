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

#include "FlyingEdges.hpp"

#include <algorithm>

using namespace complex;


///////////////////////////////////////////////////////////////////////////////
// Pass 1 of the algorithm
///////////////////////////////////////////////////////////////////////////////
void FlyingEdgesAlgorithm::pass1()
{
  // For each (j, k):
  //  - for each edge i along fixed (j, k) gridEdge, fill m_EdgeCases with
  //    cut information.
  //  - find the locations for computational trimming, xl and xr
  //  To properly find xl and xr, have to check along the x axis,
  //  the y-axis and the z-axis!
  for(size_t k = 0; k != m_NZ; ++k)
  {
    for(size_t j = 0; j != m_NY; ++j)
    {
      auto curEdgeCases = m_EdgeCases.begin() + (m_NX - 1) * (k * m_NY + j);
      auto curPointValues = m_Image.getRowIter(j, k);

      std::array<bool, 2> isGE;
      isGE[0] = (curPointValues[0] >= m_IsoVal);
      for(int i = 1; i != m_NX; ++i)
      {
        isGE[i % 2] = (curPointValues[i] >= m_IsoVal);

        curEdgeCases[i - 1] = calcCaseEdge(isGE[(i + 1) % 2], isGE[i % 2]);
      }
    }
  }

  for(size_t k = 0; k != m_NZ; ++k)
  {
    for(size_t j = 0; j != m_NY; ++j)
    {
      gridEdge& curGridEdge = m_GridEdges[k * m_NY + j];
      curGridEdge.xl = m_NX;
      for(int i = 1; i != m_NX; ++i)
      {
        // If the edge is cut
        if(isCutEdge(i - 1, j, k))
        {
          if(curGridEdge.xl == m_NX)
          {
            curGridEdge.xl = i - 1;
          }

          curGridEdge.xr = i;
        }
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Pass 2 of the algorithm
///////////////////////////////////////////////////////////////////////////////
void FlyingEdgesAlgorithm::pass2()
{
  // For each (j, k):
  //  - for each cube (i, j, k) calculate caseId and number of gridEdge cuts
  //    in the x, y and z direction.
  for(size_t k = 0; k != m_NZ - 1; ++k)
  {
    for(size_t j = 0; j != m_NY - 1; ++j)
    {
      // find adjusted trim values
      size_t xl, xr;
      calcTrimValues(xl, xr, j, k); // xl, xr set in this function

      // ge0 is owned by this (i, j, k). ge1, ge2 and ge3 are only used for
      // boundary cells.
      gridEdge& ge0 = m_GridEdges[k * m_NY + j];
      gridEdge& ge1 = m_GridEdges[k * m_NY + j + 1];
      gridEdge& ge2 = m_GridEdges[(k + 1) * m_NY + j];
      gridEdge& ge3 = m_GridEdges[(k + 1) * m_NY + j + 1];

      // ec0, ec1, ec2 and ec3 were set in pass 1. They are used
      // to calculate the cell caseId.
      auto const& ec0 = m_EdgeCases.begin() + (m_NX - 1) * (k * m_NY + j);
      auto const& ec1 = m_EdgeCases.begin() + (m_NX - 1) * (k * m_NY + j + 1);
      auto const& ec2 = m_EdgeCases.begin() + (m_NX - 1) * ((k + 1) * m_NY + j);
      auto const& ec3 = m_EdgeCases.begin() + (m_NX - 1) * ((k + 1) * m_NY + j + 1);

      // Count the number of triangles along this row of cubes.
      size_t& curTriCounter = *(m_TriCounter.begin() + k * (m_NY - 1) + j);

      auto curCubeCaseIds = m_CubeCases.begin() + (m_NX - 1) * (k * (m_NY - 1) + j);

      bool isYEnd = (j == m_NY - 2);
      bool isZEnd = (k == m_NZ - 2);

      for(size_t i = xl; i != xr; ++i)
      {
        bool isXEnd = (i == m_NX - 2);

        // using m_EdgeCases from pass 2, compute m_CubeCases for this cube
        uint8 caseId = calcCubeCase(ec0[i], ec1[i], ec2[i], ec3[i]);

        curCubeCaseIds[i] = caseId;

        // If the cube has no triangles through it
        if(caseId == 0 || caseId == 255)
        {
          continue;
        }

        curTriCounter += util::numTris[caseId];

        const bool* isCut = util::isCut[caseId]; // size 12

        ge0.xstart += isCut[0];
        ge0.ystart += isCut[3];
        ge0.zstart += isCut[8];

        // Note: Each 'gridCell' contains four m_GridEdges running along it,
        //       ge0, ge1, ge2 and ge3. Each gridCell can access it's own
        //       ge0 but ge1, ge2 and ge3 are owned by other gridCells.
        //       Accessing ge1, ge2 and ge3 leads to a race condition
        //       unless gridCell is along the boundry of the image.
        //
        //       To really make sense of the indices, it helps to draw
        //       out the following picture of a cube with the appropriate
        //       labels:
        //         v0 is at (i,   j,   k)
        //         v1       (i+1, j,   k)
        //         v2       (i+1, j+1, k)
        //         v3       (i,   j+1, k)
        //         v4       (i,   j,   k+1)
        //         v5       (i+1, j,   k+1)
        //         v6       (i+1, j+1, k+1)
        //         v7       (i,   j+1, k+1)
        //         e0  connects v0 to v1 and is parallel to the x-axis
        //         e1           v1    v2                        y
        //         e2           v2    v3                        x
        //         e3           v0    v3                        y
        //         e4           v4    v5                        x
        //         e5           v5    v6                        y
        //         e6           v6    v7                        x
        //         e7           v4    v7                        y
        //         e8           v0    v4                        z
        //         e9           v1    v5                        z
        //         e10          v3    v7                        z
        //         e11          v2    v6                        z

        // Handle cubes along the edge of the image
        if(isXEnd)
        {
          ge0.ystart += isCut[1];
          ge0.zstart += isCut[9];
        }
        if(isYEnd)
        {
          ge1.xstart += isCut[2];
          ge1.zstart += isCut[10];
        }
        if(isZEnd)
        {
          ge2.xstart += isCut[4];
          ge2.ystart += isCut[7];
        }

        if(isXEnd and isYEnd)
        {
          ge1.zstart += isCut[11];
        }
        if(isXEnd and isZEnd)
        {
          ge2.ystart += isCut[5];
        }
        if(isYEnd and isZEnd)
        {
          ge3.xstart += isCut[6];
        }
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Pass 3 of the algorithm
///////////////////////////////////////////////////////////////////////////////
void FlyingEdgesAlgorithm::pass3()
{
  // Accumulate triangles into triCounter
  size_t tmp;
  size_t triAccum = 0;
  for(size_t k = 0; k != m_NZ - 1; ++k)
  {
    for(size_t j = 0; j != m_NY - 1; ++j)
    {
      size_t& curTriCounter = m_TriCounter[k * (m_NY - 1) + j];

      tmp = curTriCounter;
      curTriCounter = triAccum;
      triAccum += tmp;
    }
  }

  // accumulate points, filling out starting locations of each gridEdge
  // in the process.
  size_t pointAccum = 0;
  for(size_t k = 0; k != m_NZ; ++k)
  {
    for(size_t j = 0; j != m_NY; ++j)
    {
      gridEdge& curGridEdge = m_GridEdges[k * m_NY + j];

      tmp = curGridEdge.xstart;
      curGridEdge.xstart = pointAccum;
      pointAccum += tmp;

      tmp = curGridEdge.ystart;
      curGridEdge.ystart = pointAccum;
      pointAccum += tmp;

      tmp = curGridEdge.zstart;
      curGridEdge.zstart = pointAccum;
      pointAccum += tmp;
    }
  }

  /* Saving jic. Same thing as above just scanned in different order
   *
      size_t pointAccum = 0;
      for(size_t k = 0; k != m_NZ; ++k) {
      for(size_t j = 0; j != m_NY; ++j)
      {
          gridEdge& curGridEdge = m_GridEdges[k*m_NY + j];

          tmp = curGridEdge.xstart;
          curGridEdge.xstart = pointAccum;
          pointAccum += tmp;
      }}

      for(size_t k = 0; k != m_NZ; ++k) {
      for(size_t j = 0; j != m_NY; ++j)
      {
          gridEdge& curGridEdge = m_GridEdges[k*m_NY + j];

          tmp = curGridEdge.ystart;
          curGridEdge.ystart = pointAccum;
          pointAccum += tmp;
      }}

      for(size_t k = 0; k != m_NZ; ++k) {
      for(size_t j = 0; j != m_NY; ++j)
      {
          gridEdge& curGridEdge = m_GridEdges[k*m_NY + j];

          tmp = curGridEdge.zstart;
          curGridEdge.zstart = pointAccum;
          pointAccum += tmp;
      }}
  */

  m_Points = std::vector<std::array<float32, 3>>(pointAccum);
  m_Normals = std::vector<std::array<float32, 3>>(pointAccum);
  m_Tris = std::vector<std::array<size_t, 3>>(triAccum);
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Pass 4 of the algorithm
///////////////////////////////////////////////////////////////////////////////
void FlyingEdgesAlgorithm::pass4()
{
  // For each (j, k):
  //  - For each cube at i, fill out points, normals and triangles owned by
  //    the cube. Each cube is in charge of filling out e0, e3 and e8. Only
  //    in edge cases does it also fill out other edges.
  for(size_t k = 0; k != m_NZ - 1; ++k)
  {
    for(size_t j = 0; j != m_NY - 1; ++j)
    {
      // find adjusted trim values
      size_t xl, xr;
      calcTrimValues(xl, xr, j, k); // xl, xr set in this function

      if(xl == xr)
        continue;

      size_t triIdx = m_TriCounter[k * (m_NY - 1) + j];
      auto curCubeCaseIds = m_CubeCases.begin() + (m_NX - 1) * (k * (m_NY - 1) + j);

      gridEdge const& ge0 = m_GridEdges[k * m_NY + j];
      gridEdge const& ge1 = m_GridEdges[k * m_NY + j + 1];
      gridEdge const& ge2 = m_GridEdges[(k + 1) * m_NY + j];
      gridEdge const& ge3 = m_GridEdges[(k + 1) * m_NY + j + 1];

      size_t x0counter = 0;
      size_t y0counter = 0;
      size_t z0counter = 0;

      size_t x1counter = 0;
      size_t z1counter = 0;

      size_t x2counter = 0;
      size_t y2counter = 0;

      size_t x3counter = 0;

      bool isYEnd = (j == m_NY - 2);
      bool isZEnd = (k == m_NZ - 2);

      for(size_t i = xl; i != xr; ++i)
      {
        bool isXEnd = (i == m_NX - 2);

        uint8 caseId = curCubeCaseIds[i];

        if(caseId == 0 || caseId == 255)
        {
          continue;
        }

        const bool* isCut = util::isCut[caseId]; // has 12 elements

        // Most of the information contained in pointCube, isovalCube
        // and gradCube will be used--but not necessarily all. It has
        // not been tested whether or not obtaining only the information
        // needed will provide a significant speedup--but
        // most likely not.
        cube pointCube = m_Image.getPosCube(i, j, k);
        float32Cube isovalCube = m_Image.getValsCube(i, j, k);
        cube gradCube = m_Image.getGradCube(i, j, k);

        // Add Points and normals.
        // Calculate global indices for triangles
        std::array<size_t, 12> globalIdxs;

        if(isCut[0])
        {
          size_t idx = ge0.xstart + x0counter;
          m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 0);
          m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 0);
          globalIdxs[0] = idx;
          ++x0counter;
        }

        if(isCut[3])
        {
          size_t idx = ge0.ystart + y0counter;
          m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 3);
          m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 3);
          globalIdxs[3] = idx;
          ++y0counter;
        }

        if(isCut[8])
        {
          size_t idx = ge0.zstart + z0counter;
          m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 8);
          m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 8);
          globalIdxs[8] = idx;
          ++z0counter;
        }

        // Note:
        //   e1, e5, e9 and e11 will be visited in the next iteration
        //   when they are e3, e7, e8 and 10 respectively. So don't
        //   increment their counters. When the cube is an edge cube,
        //   their counters don't need to be incremented because they
        //   won't be used agin.

        // Manage boundary cases if needed. Otherwise just update
        // globalIdx.
        if(isCut[1])
        {
          size_t idx = ge0.ystart + y0counter;
          if(isXEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 1);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 1);
            // y0counter counter doesn't need to be incremented
            // because it won't be used again.
          }
          globalIdxs[1] = idx;
        }

        if(isCut[9])
        {
          size_t idx = ge0.zstart + z0counter;
          if(isXEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 9);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 9);
            // z0counter doesn't need to in incremented.
          }
          globalIdxs[9] = idx;
        }

        if(isCut[2])
        {
          size_t idx = ge1.xstart + x1counter;
          if(isYEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 2);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 2);
          }
          globalIdxs[2] = idx;
          ++x1counter;
        }

        if(isCut[10])
        {
          size_t idx = ge1.zstart + z1counter;

          if(isYEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 10);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 10);
          }
          globalIdxs[10] = idx;
          ++z1counter;
        }

        if(isCut[4])
        {
          size_t idx = ge2.xstart + x2counter;
          if(isZEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 4);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 4);
          }
          globalIdxs[4] = idx;
          ++x2counter;
        }

        if(isCut[7])
        {
          size_t idx = ge2.ystart + y2counter;
          if(isZEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 7);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 7);
          }
          globalIdxs[7] = idx;
          ++y2counter;
        }

        if(isCut[11])
        {
          size_t idx = ge1.zstart + z1counter;
          if(isXEnd and isYEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 11);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 11);
            // z1counter does not need to be incremented.
          }
          globalIdxs[11] = idx;
        }

        if(isCut[5])
        {
          size_t idx = ge2.ystart + y2counter;
          if(isXEnd and isZEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 5);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 5);
            // y2 counter does not need to be incremented.
          }
          globalIdxs[5] = idx;
        }

        if(isCut[6])
        {
          size_t idx = ge3.xstart + x3counter;
          if(isYEnd and isZEnd)
          {
            m_Points[idx] = interpolateOnCube(pointCube, isovalCube, 6);
            m_Normals[idx] = interpolateOnCube(gradCube, isovalCube, 6);
          }
          globalIdxs[6] = idx;
          ++x3counter;
        }

        // Add triangles
        const char* caseTri = util::caseTriangles[caseId]; // size 16
        for(int idx = 0; caseTri[idx] != -1; idx += 3)
        {
          m_Tris[triIdx][0] = globalIdxs[caseTri[idx]];
          m_Tris[triIdx][1] = globalIdxs[caseTri[idx + 1]];
          m_Tris[triIdx][2] = globalIdxs[caseTri[idx + 2]];
          ++triIdx;
        }
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Don't copy points, normals and m_Tris but move the output into a TrianlgeMesh.
///////////////////////////////////////////////////////////////////////////////
util::TriangleMesh FlyingEdgesAlgorithm::moveOutput()
{
  return util::TriangleMesh(std::move(m_Points), std::move(m_Normals), std::move(m_Tris));
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Private helper functions
///////////////////////////////////////////////////////////////////////////////

bool FlyingEdgesAlgorithm::isCutEdge(size_t const& i, size_t const& j, size_t const& k) const
{
  // Assuming m_EdgeCases are all set
  size_t edgeCaseIdx = k * (m_NX - 1) * m_NY + j * (m_NX - 1) + i;
  if(m_EdgeCases[edgeCaseIdx] == 1 || m_EdgeCases[edgeCaseIdx] == 2)
  {
    return true;
  }

  if(j != m_NY - 1)
  {
    size_t edgeCaseIdxY = k * (m_NX - 1) * m_NY + (j + 1) * (m_NX - 1) + i;

    // If (edgeCaseX, edgeCaseY) is (0, 1), (1, 2), (2, 3), (0, 3)
    //                              (1, 0), (2, 1), (3, 2), (3, 0)
    // and not the other options of (0, 2), (1, 3),
    //                              (2, 0), (3, 1)
    // then the edge along the y axis is cut.
    // So check to see if edgeCaseX + edgeCaseY is odd.
    if((m_EdgeCases[edgeCaseIdx] + m_EdgeCases[edgeCaseIdxY]) % 2 == 1)
    {
      return true;
    }
  }

  if(k != m_NZ - 1)
  {
    size_t edgeCaseIdxZ = (k + 1) * (m_NX - 1) * m_NY + j * (m_NX - 1) + i;

    // Same as above. If it is odd, then there is a cut except this
    // time along the z axis.
    if((m_EdgeCases[edgeCaseIdx] + m_EdgeCases[edgeCaseIdxZ]) % 2 == 1)
    {
      return true;
    }
  }

  return false;
}

inline uint8 FlyingEdgesAlgorithm::calcCaseEdge(bool const& prevEdge, bool const& currEdge) const
{
  // o -- is greater than or equal to
  // case 0: (i-1) o-----o (i) | (_,j,k)
  // case 1: (i-1) x-----o (i) | (_,j+1,k)
  // case 2: (i-1) o-----x (i) | (_,j,k+1)
  // case 3: (i-1) x-----x (i) | (_,j+1,k+1)
  if(prevEdge && currEdge)
    return 0;
  if(!prevEdge && currEdge)
    return 1;
  if(prevEdge && !currEdge)
    return 2;
  else // !prevEdge && !currEdge
    return 3;
}

inline uint8 FlyingEdgesAlgorithm::calcCubeCase(uint8 const& ec0, uint8 const& ec1, uint8 const& ec2, uint8 const& ec3) const
{
  // ec0 | (_,j,k)
  // ec1 | (_,j+1,k)
  // ec2 | (_,j,k+1)
  // ec3 | (_,j+1,k+1)

  uint8 caseId = 0;
  if((ec0 == 0) || (ec0 == 2)) // 0 | (i,j,k)
    caseId |= 1;
  if((ec0 == 0) || (ec0 == 1)) // 1 | (i+1,j,k)
    caseId |= 2;
  if((ec1 == 0) || (ec1 == 1)) // 2 | (i+1,j+1,k)
    caseId |= 4;
  if((ec1 == 0) || (ec1 == 2)) // 3 | (i,j+1,k)
    caseId |= 8;
  if((ec2 == 0) || (ec2 == 2)) // 4 | (i,j,k+1)
    caseId |= 16;
  if((ec2 == 0) || (ec2 == 1)) // 5 | (i+1,j,k+1)
    caseId |= 32;
  if((ec3 == 0) || (ec3 == 1)) // 6 | (i+1,j+1,k+1)
    caseId |= 64;
  if((ec3 == 0) || (ec3 == 2)) // 7 | (i,j+1,k+1)
    caseId |= 128;
  return caseId;
}

inline void FlyingEdgesAlgorithm::calcTrimValues(size_t& xl, size_t& xr, size_t const& j, size_t const& k) const
{
  gridEdge const& ge0 = m_GridEdges[k * m_NY + j];
  gridEdge const& ge1 = m_GridEdges[k * m_NY + j + 1];
  gridEdge const& ge2 = m_GridEdges[(k + 1) * m_NY + j];
  gridEdge const& ge3 = m_GridEdges[(k + 1) * m_NY + j + 1];

  xl = size_t(std::min({ge0.xl, ge1.xl, ge2.xl, ge3.xl}));
  xr = size_t(std::max({ge0.xr, ge1.xr, ge2.xr, ge3.xr}));

  if(xl > xr)
    xl = xr;
}

inline std::array<float32, 3> FlyingEdgesAlgorithm::interpolateOnCube(cube const& pts, float32Cube const& isovals, uint8 const& edge) const
{
  uint8 i0 = util::edgeVertices[edge][0];
  uint8 i1 = util::edgeVertices[edge][1];

  float32 weight = (m_IsoVal - isovals[i0]) / (isovals[i1] - isovals[i0]);
  return interpolate(pts[i0], pts[i1], weight);
}

inline std::array<float32, 3> FlyingEdgesAlgorithm::interpolate(std::array<float32, 3> const& a, std::array<float32, 3> const& b, float32 const& weight) const
{
  std::array<float32, 3> ret;
  ret[0] = a[0] + (weight * (b[0] - a[0]));
  ret[1] = a[1] + (weight * (b[1] - a[1]));
  ret[2] = a[2] + (weight * (b[2] - a[2]));
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
