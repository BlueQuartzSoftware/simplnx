#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindEuclideanDistMapInputValues inputValues;

  inputValues.CalcManhattanDist = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  inputValues.DoBoundaries = filterArgs.value<bool>(k_DoBoundaries_Key);
  inputValues.DoTripleLines = filterArgs.value<bool>(k_DoTripleLines_Key);
  inputValues.DoQuadPoints = filterArgs.value<bool>(k_DoQuadPoints_Key);
  inputValues.SaveNearestNeighbors = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.GBDistancesArrayName = filterArgs.value<DataPath>(k_GBDistancesArrayName_Key);
  inputValues.TJDistancesArrayName = filterArgs.value<DataPath>(k_TJDistancesArrayName_Key);
  inputValues.QPDistancesArrayName = filterArgs.value<DataPath>(k_QPDistancesArrayName_Key);
  inputValues.NearestNeighborsArrayName = filterArgs.value<DataPath>(k_NearestNeighborsArrayName_Key);

  return FindEuclideanDistMap(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT FindEuclideanDistMapInputValues
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

class CORE_EXPORT FindEuclideanDistMap
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

} // namespace complex
