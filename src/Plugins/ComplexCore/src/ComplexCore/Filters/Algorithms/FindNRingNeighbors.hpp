#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/IFilter.hpp"

#include <set>

namespace complex
{
class TriangleGeom;

struct COMPLEXCORE_EXPORT FindNRingNeighborsInputValues
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

class COMPLEXCORE_EXPORT FindNRingNeighbors
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
} // namespace complex
