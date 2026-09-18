#pragma once

#include "simplnx/Common/Types.hpp"

#include <cmath>
#include <vector>

/**
 * @brief Shared neighbor-offset sets for grayscale morphological reconstruction sweeps. Both the morphological
 *        reconstruction engine and the regional extrema engine (which reuses the same neighborhoods to drive its
 *        own sweeps) need these; they live in this minimal header so the extrema engine does not have to depend
 *        on the full reconstruction engine.
 */

namespace nx::core::ImageProcessing::detail
{
// A signed 3D neighbor offset (dx,dy,dz); the reconstruction neighborhoods exclude the center {0,0,0}.
struct ReconOffset
{
  int32 dx;
  int32 dy;
  int32 dz;
};

/**
 * @brief The three neighbor-offset sets ITK's reconstruction needs per Vincent phase, built from the
 *        connectivity flag: @c all (full neighborhood, center excluded) used by the FIFO flood; @c previous (the
 *        raster-order-earlier half) used by the forward raster sweep; @c later (the raster-order-later half) used
 *        by the reverse anti-raster sweep. Raster order is Z slowest, X fastest, so an offset is "previous" iff it
 *        is lexicographically less than (0,0,0) in (dz,dy,dx); previous and later partition all\{center}.
 *
 * Face connectivity (fullyConnected=false): the 6 axis neighbors (4 in 2D). Full (true): all 26 (8 in 2D).
 */
struct ReconOffsets
{
  std::vector<ReconOffset> all;
  std::vector<ReconOffset> previous;
  std::vector<ReconOffset> later;
};

inline ReconOffsets MakeReconstructionOffsets(bool fullyConnected)
{
  ReconOffsets result;
  for(int32 dz = -1; dz <= 1; ++dz)
  {
    for(int32 dy = -1; dy <= 1; ++dy)
    {
      for(int32 dx = -1; dx <= 1; ++dx)
      {
        if(dx == 0 && dy == 0 && dz == 0)
        {
          continue; // center excluded
        }
        if(!fullyConnected && (std::abs(dx) + std::abs(dy) + std::abs(dz)) != 1)
        {
          continue; // face connectivity: exactly one axis nonzero
        }
        const ReconOffset off{dx, dy, dz};
        result.all.push_back(off);
        const bool isPrevious = (dz < 0) || (dz == 0 && dy < 0) || (dz == 0 && dy == 0 && dx < 0);
        if(isPrevious)
        {
          result.previous.push_back(off);
        }
        else
        {
          result.later.push_back(off);
        }
      }
    }
  }
  return result;
}
} // namespace nx::core::ImageProcessing::detail
