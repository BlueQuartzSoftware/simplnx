// MMSurfaceNet.h
//
// Application Programming Interface (API)
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA
//
// MMSurfaceNet
// A free, open-source C++ implementation of 3D SurfaceNets that supports multiple
// materials (up to 62235) represented as indexed labels (0 to 65534)
//
// Disclaimer
// The Software is provided "AS IS" and neither Brigham nor any
// contributor to the software(each a "Contributor") shall have any
// obligation to provide maintenance, support, updates, enhancements
// or modifications thereto. BRIGHAM AND ALL CONTRIBUTORS SPECIFICALLY
// DISCLAIM ALL EXPRESS AND IMPLIED WARRANTIES OF ANY KIND INCLUDING,
// BUT NOT LIMITED TO, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
// BRIGHAM OR ANY CONTRIBUTOR BE LIABLE TO ANY PARTY FOR DIRECT,
// INDIRECT, SPECIAL, INCIDENTAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY ARISING IN ANY WAY
// RELATED TO THE SOFTWARE, EVEN IF BRIGHAM OR ANY CONTRIBUTOR HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.TO THE MAXIMUM
// EXTENT NOT PROHIBITED BY LAW OR REGULATION, YOU FURTHER ASSUME ALL
// LIABILITY FOR YOUR USE, REPRODUCTION, MAKING OF DERIVATIVE WORKS,
// DISPLAY, LICENSE OR DISTRIBUTION OF THE SOFTWARE AND AGREE TO
// INDEMNIFY AND HOLD HARMLESS BRIGHAM AND ALL CONTRIBUTORS FROM AND
// AGAINST ANY AND ALL CLAIMS, SUITS, ACTIONS, DEMANDS AND JUDGMENTS
// ARISING THEREFROM.
//
// Additional Information
// This implementation is described in:
//    S. Frisken, "SurfaceNets for Multi-Label Segmentations with
//    Preservation of Sharp Boundaries", Journal of Comptuer Graphics
//    Techniques, 2021.
//
// SurfaceNets was originally presented in:
//    S. Gibson (a.k.a. Sarah Frisken), Constrained Elastic SurfaceNets: Generating
//    Smooth Surfaces from Binary Segmented Data, Proc. Medical Image Computation
//    and Computer Assisted Interventions, Oct. 1998, pp. 888-898.
//
//    R. Perry and S. Frisken, Kizamu: A System for Sculpting Digital Characters,
//    Proc. SIGGRAPH 2001, pp. 47-56, 2001.
//
// SurfaceNets was patented by Mitsubishi Electric July 4, 2000, US 6,084,593. This
// patent has since expired.

#ifndef MM_SURFACE_NET_H
#define MM_SURFACE_NET_H

#include <string>
#include <vector>

class MMCellMap;

class MMSurfaceNet
{
public:
  MMSurfaceNet(int32_t* labels, int arraySize[3], float voxelSize[3]);
  ~MMSurfaceNet();

  // Surface smoothing (relaxation)
  struct RelaxAttrs
  {
    int numRelaxIterations;      // More iterations --> smoother and slower
    float relaxFactor;           // Range (0.0, 1.0); larger --> faster but less stable
    float maxDistFromCellCenter; // Maximun displacement of relaxed surface in voxel units
  };
  void relax(const RelaxAttrs relaxAttrs);
  void reset();

  // Get the unique material labels for this SurfaceNet
  std::vector<int> labels();

  // Label used internally. Not available as a material index.
  enum ReservedLabel
  {
    Pading = 65535
  };

  MMCellMap* getCellMap() const;

private:
  friend class MMGeometryGL;
  friend class MMGeometryOBJ;

  MMCellMap* m_cellMap;
};

#endif
