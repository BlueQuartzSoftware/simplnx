#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeSurfaceFeaturesInputValues
 * @brief Stores paths and surface-marking options.
 *
 * A feature is surface when one voxel reaches the geometry boundary. Feature Id
 * 0 neighbors also mark a feature when the selected option is true.
 */
struct SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesInputValues
{
  AttributeMatrixSelectionParameter::ValueType FeatureAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  BoolParameter::ValueType MarkFeature0Neighbors;
  DataObjectNameParameter::ValueType SurfaceFeaturesArrayName;
};

/**
 * @class ComputeSurfaceFeatures
 * @brief Dispatches surface-feature labeling by Feature Id storage.
 *
 * The cell-scale Feature Id array drives dispatch because face-neighbor reads can
 * thrash disk chunks. The feature-level output does not affect selection.
 *
 * @see ComputeSurfaceFeaturesDirect, ComputeSurfaceFeaturesScanline, DispatchAlgorithm
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeatures
{
public:
  /**
   * @brief Creates a surface-feature dispatcher.
   * @param dataStructure Provides the selected arrays and geometry.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later slices when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  ComputeSurfaceFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeSurfaceFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~ComputeSurfaceFeatures() noexcept;

  ComputeSurfaceFeatures(const ComputeSurfaceFeatures&) = delete;
  ComputeSurfaceFeatures(ComputeSurfaceFeatures&&) noexcept = delete;
  ComputeSurfaceFeatures& operator=(const ComputeSurfaceFeatures&) = delete;
  ComputeSurfaceFeatures& operator=(ComputeSurfaceFeatures&&) noexcept = delete;

  /**
   * @brief Dispatches surface-feature labeling.
   * @return Error from the selected implementation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
