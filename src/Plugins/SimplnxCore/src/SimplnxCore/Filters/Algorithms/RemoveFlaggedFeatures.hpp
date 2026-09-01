#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @enum Functionality
 * @brief Selects feature extraction, removal, or both operations.
 */
enum class Functionality : uint64
{
  Remove = 0,            ///< Removes selected features from the source geometry.
  Extract = 1,           ///< Extracts each selected feature to a cropped geometry.
  ExtractThenRemove = 2, ///< Extracts selected features before source removal.
};

/**
 * @struct RemoveFlaggedFeaturesInputValues
 * @brief Stores operation settings, paths, names, and ignored cell arrays.
 */
struct SIMPLNXCORE_EXPORT RemoveFlaggedFeaturesInputValues
{
  bool FillRemovedFeatures;
  uint64 ExtractFeatures;
  DataPath FeatureIdsArrayPath;
  DataPath FlaggedFeaturesArrayPath;
  DataPath ImageGeometryPath;
  DataPath TempBoundsPath;
  std::string CreatedImageGeometryPrefix;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class RemoveFlaggedFeatures
 * @brief Dispatches flagged-feature extraction and removal from participating storage.
 *
 * The dispatcher always checks Feature IDs. When removal fills gaps, it also checks
 * each companion cell array that can receive neighbor data.
 *
 * A zero Feature ID participates in convergence but is not filled. Gap filling
 * does not terminate while zero Feature IDs remain.
 *
 * @see RemoveFlaggedFeaturesDirect
 * @see RemoveFlaggedFeaturesScanline
 */
class SIMPLNXCORE_EXPORT RemoveFlaggedFeatures
{
public:
  /**
   * @brief Creates a flagged-feature dispatcher.
   * @param dataStructure Provides source geometry, feature data, and outputs.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later extraction or removal work when true.
   * @param inputValues Specifies validated settings and paths. The caller must keep
   * this object alive for the dispatcher lifetime.
   */
  RemoveFlaggedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~RemoveFlaggedFeatures() noexcept;

  RemoveFlaggedFeatures(const RemoveFlaggedFeatures&) = delete;
  RemoveFlaggedFeatures(RemoveFlaggedFeatures&&) noexcept = delete;
  RemoveFlaggedFeatures& operator=(const RemoveFlaggedFeatures&) = delete;
  RemoveFlaggedFeatures& operator=(RemoveFlaggedFeatures&&) noexcept = delete;

  /**
   * @brief Selects direct or scanline execution from target-array storage.
   * @return First reported removal or I/O error, or success after cancellation.
   *
   * Delegated preflight failures throw. Delegated execute failures are not inspected.
   * Cancellation and errors can retain extracted geometries or modified source arrays.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
