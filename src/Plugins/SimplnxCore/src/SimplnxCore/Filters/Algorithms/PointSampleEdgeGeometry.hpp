#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include <string_view>

namespace nx::core
{
/**
 * @namespace nx::core::detail
 * @brief Contains internal sample-layout arithmetic.
 */
namespace detail
{
/**
 * @brief Adds final sample counts without overflow.
 * @param total Gives the accumulated sample count.
 * @param increment Gives the next edge sample count.
 * @return The sum or an aggregate sample count error.
 */
SIMPLNXCORE_EXPORT Result<usize> AddSampleCounts(usize total, usize increment);
/**
 * @brief Checks the signed sample-index representation.
 * @param count Gives the final accumulated sample count.
 * @return An error if the count exceeds the signed index limit.
 */
SIMPLNXCORE_EXPORT Result<> CheckSampleIndexCount(usize count);
/**
 * @brief Checks sample values and bytes without allocating an array.
 * @param tuples Gives the final sample count.
 * @param components Gives the array component count.
 * @param elementBytes Gives the size of one value in bytes.
 * @param arrayName Identifies the consumed output in an error message.
 * @return A value-count or byte-count error, otherwise success.
 */
SIMPLNXCORE_EXPORT Result<> CheckSampleArraySize(usize tuples, usize components, usize elementBytes, std::string_view arrayName);
} // namespace detail

struct SIMPLNXCORE_EXPORT PointSampleEdgeGeometryInputValues
{
  float32 ScanVectorSamplingRes;
  DataPath ScanVectorGeometryPath;
  DataPath SampledVertexGeometryPath;
  bool CalculateCumulativeSampleDistance;
  std::string CumulativeSampleDistanceArrayName;
  std::string EdgeIdsArrayName;
  DataPath pVertexGroupDataPath;
  MultiArraySelectionParameter::ValueType pSelectedDataArrayPaths;
  MultiArraySelectionParameter::ValueType pCreatedDataArrayPaths;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT PointSampleEdgeGeometry
{
public:
  PointSampleEdgeGeometry(DataStructure& dataStructure, PointSampleEdgeGeometryInputValues* inputValues, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel);
  ~PointSampleEdgeGeometry() noexcept;

  PointSampleEdgeGeometry(const PointSampleEdgeGeometry&) = delete;
  PointSampleEdgeGeometry(PointSampleEdgeGeometry&&) noexcept = delete;
  PointSampleEdgeGeometry& operator=(const PointSampleEdgeGeometry&) = delete;
  PointSampleEdgeGeometry& operator=(PointSampleEdgeGeometry&&) noexcept = delete;

  /**
   * @brief Samples edges and transfers selected attributes.
   * @pre Sampling spacing is finite and positive; source and output selections are valid.
   * @return Final sample-count, layout, transfer or output resize errors.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const PointSampleEdgeGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
