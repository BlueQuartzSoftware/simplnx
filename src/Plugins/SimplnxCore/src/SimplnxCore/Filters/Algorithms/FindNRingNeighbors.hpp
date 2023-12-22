#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <set>

namespace nx::core
{
class TriangleGeom;

struct SIMPLNXCORE_EXPORT FindNRingNeighborsInputValues
{
  TriangleGeom* TriangleGeomPtr = nullptr;
  int64 TriangleId = {-1};
  int32 RegionId0 = {0};
  int32 RegionId1 = {0};
  int64 Ring = {2};
  Int32Array* FaceLabelsArray = nullptr;
  bool WriteBinaryFile = {false};
  bool WriteConformalMesh = {true};
};

class SIMPLNXCORE_EXPORT FindNRingNeighbors
{
public:
  using UniqueFaceIds_t = std::set<int64>;

  FindNRingNeighbors(FindNRingNeighborsInputValues* inputValues);
  ~FindNRingNeighbors() noexcept;

  FindNRingNeighbors(const FindNRingNeighbors&) = delete;
  FindNRingNeighbors(FindNRingNeighbors&&) noexcept = delete;
  FindNRingNeighbors& operator=(const FindNRingNeighbors&) = delete;
  FindNRingNeighbors& operator=(FindNRingNeighbors&&) noexcept = delete;

  Result<> operator()(const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel);

  UniqueFaceIds_t getNRingTriangles() const;

private:
  const FindNRingNeighborsInputValues* m_InputValues = nullptr;
  UniqueFaceIds_t m_NRingTriangles;
};
} // namespace nx::core
