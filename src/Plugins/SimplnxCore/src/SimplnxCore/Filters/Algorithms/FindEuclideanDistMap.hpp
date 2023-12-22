#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindEuclideanDistMapInputValues
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT FindEuclideanDistMap
{
public:
  FindEuclideanDistMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindEuclideanDistMapInputValues* inputValues);
  ~FindEuclideanDistMap() noexcept;

  FindEuclideanDistMap(const FindEuclideanDistMap&) = delete;
  FindEuclideanDistMap(FindEuclideanDistMap&&) noexcept = delete;
  FindEuclideanDistMap& operator=(const FindEuclideanDistMap&) = delete;
  FindEuclideanDistMap& operator=(FindEuclideanDistMap&&) noexcept = delete;

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
  const FindEuclideanDistMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
