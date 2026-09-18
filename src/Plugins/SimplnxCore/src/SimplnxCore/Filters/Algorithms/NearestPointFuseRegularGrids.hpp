#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct NearestPointFuseRegularGridsInputValues
 * @brief Stores geometry paths, AttributeMatrix paths, and the fill value.
 */
struct SIMPLNXCORE_EXPORT NearestPointFuseRegularGridsInputValues
{
  DataPath ReferenceGeometryPath;
  DataPath SamplingGeometryPath;
  DataPath ReferenceCellAttributeMatrixPath;
  DataPath SamplingCellAttributeMatrixPath;
  float64 fillValue;
};

/**
 * @class NearestPointFuseRegularGrids
 * @brief Resamples sampling-grid cell arrays onto a reference ImageGeom.
 *
 * The dispatcher checks all numeric and Boolean source/destination array pairs.
 * Resident pairs use parallel direct access. Disk-backed pairs use bounded row I/O.
 */
class SIMPLNXCORE_EXPORT NearestPointFuseRegularGrids
{
public:
  /**
   * @brief Creates a nearest-point grid-fusion dispatcher.
   * @param dataStructure Provides both image geometries and their cell arrays.
   * @param mesgHandler Is retained for the dispatched interface.
   * @param shouldCancel Stops later arrays or slices when true.
   * @param inputValues Specifies validated paths and the fill value. The caller
   * must keep this object alive for the dispatcher lifetime.
   */
  NearestPointFuseRegularGrids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, NearestPointFuseRegularGridsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~NearestPointFuseRegularGrids() noexcept;

  NearestPointFuseRegularGrids(const NearestPointFuseRegularGrids&) = delete;
  NearestPointFuseRegularGrids(NearestPointFuseRegularGrids&&) noexcept = delete;
  NearestPointFuseRegularGrids& operator=(const NearestPointFuseRegularGrids&) = delete;
  NearestPointFuseRegularGrids& operator=(NearestPointFuseRegularGrids&&) noexcept = delete;

  /**
   * @brief Selects direct or row-buffered fusion from participating storage.
   * @return Error for zero sampling spacing or bulk I/O, or success after cancellation.
   *
   * Each reference lattice coordinate selects its containing sampling cell. Values
   * outside the sampling extent receive the fill value converted to the array type.
   * Cancellation or an I/O error can retain partial destination arrays.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const NearestPointFuseRegularGridsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
