#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeEuclideanDistMapInputValues
{
  bool CalcManhattanDist;
  bool DoBoundaries;
  bool DoTripleLines;
  bool DoQuadPoints;
  bool SaveNearestNeighbors;
  DataPath FeatureIdsArrayPath;
  DataPath GBDistancesArrayName;
  DataPath TJDistancesArrayName;
  DataPath QPDistancesArrayName;
  DataPath NearestNeighborsArrayName;
  DataPath InputImageGeometry;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeEuclideanDistMap
{
public:
  ComputeEuclideanDistMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeEuclideanDistMapInputValues* inputValues);
  ~ComputeEuclideanDistMap() noexcept;

  ComputeEuclideanDistMap(const ComputeEuclideanDistMap&) = delete;
  ComputeEuclideanDistMap(ComputeEuclideanDistMap&&) noexcept = delete;
  ComputeEuclideanDistMap& operator=(const ComputeEuclideanDistMap&) = delete;
  ComputeEuclideanDistMap& operator=(ComputeEuclideanDistMap&&) noexcept = delete;

  using EnumType = uint32_t;

  enum class MapType : EnumType
  {
    FeatureBoundary = 0, //!<
    TripleJunction = 1,  //!<
    QuadPoint = 2,       //!<
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeEuclideanDistMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
