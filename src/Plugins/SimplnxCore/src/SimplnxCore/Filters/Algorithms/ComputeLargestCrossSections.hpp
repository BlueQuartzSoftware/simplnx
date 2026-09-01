#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeLargestCrossSectionsInputValues
 * @brief Stores validated paths and the selected cross-section plane.
 */
struct SIMPLNXCORE_EXPORT ComputeLargestCrossSectionsInputValues
{
  ChoicesParameter::ValueType Plane;
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath LargestCrossSectionsArrayPath;
};

/**
 * @class ComputeLargestCrossSections
 * @brief Computes each feature's largest cross-section perpendicular to a
 * selected image axis by dispatching to storage-appropriate implementations.
 */
class SIMPLNXCORE_EXPORT ComputeLargestCrossSections
{
public:
  /**
   * @brief Creates a cross-section dispatcher.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later planes when true.
   * @param inputValues Specifies validated paths and the plane. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  ComputeLargestCrossSections(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeLargestCrossSectionsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~ComputeLargestCrossSections() noexcept;

  ComputeLargestCrossSections(const ComputeLargestCrossSections&) = delete;
  ComputeLargestCrossSections(ComputeLargestCrossSections&&) noexcept = delete;
  ComputeLargestCrossSections& operator=(const ComputeLargestCrossSections&) = delete;
  ComputeLargestCrossSections& operator=(ComputeLargestCrossSections&&) noexcept = delete;

  /**
   * @brief Dispatches the cross-section calculation.
   * @return Error from the selected implementation.
   *
   * Feature Id storage selects the direct or bulk-plane implementation.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeLargestCrossSectionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
