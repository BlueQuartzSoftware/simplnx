#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <set>

namespace nx::core
{
class TriangleGeom;

struct SIMPLNXCORE_EXPORT FindNRingNeighborsInputValues
{
  TriangleGeom* triangle_geom_ptr = nullptr;
  int64 triangle_id = {-1};
  int32 region_id0 = {0};
  int32 region_id1 = {0};
  int64 ring = {2};
  Int32ArrayType* face_labels_array = nullptr;
  bool write_binary_file = {false};
  bool write_conformal_mesh = {true};
};

class SIMPLNXCORE_EXPORT FindNRingNeighbors
{
public:
  using UniqueFaceIdsTType = std::set<IGeometry::MeshIndexType>;

  FindNRingNeighbors(FindNRingNeighborsInputValues* inputValues);
  ~FindNRingNeighbors() noexcept;

  FindNRingNeighbors(const FindNRingNeighbors&) = delete;
  FindNRingNeighbors(FindNRingNeighbors&&) noexcept = delete;
  FindNRingNeighbors& operator=(const FindNRingNeighbors&) = delete;
  FindNRingNeighbors& operator=(FindNRingNeighbors&&) noexcept = delete;

  Result<> operator()(const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel);

  UniqueFaceIdsTType getNRingTriangles() const;

private:
  const FindNRingNeighborsInputValues* m_InputValues = nullptr;
  UniqueFaceIdsTType m_NRingTriangles;
};
} // namespace nx::core
