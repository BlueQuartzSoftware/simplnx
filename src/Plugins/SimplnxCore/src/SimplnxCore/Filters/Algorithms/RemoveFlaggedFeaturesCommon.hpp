#pragma once

#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <fmt/format.h>

#include <any>
#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace nx::core
{

inline std::string FirstRemoveFlaggedFeaturesErrorMessage(const std::vector<Error>& errors)
{
  if(errors.empty())
  {
    return "(no error message)";
  }
  std::string message = fmt::format("[{}] {}", errors[0].code, errors[0].message);
  if(errors.size() > 1)
  {
    message += fmt::format(" (+{} more)", errors.size() - 1);
  }
  return message;
}

/**
 * @class ExtractionCropFilter
 * @brief Crops extracted cells without copying aliases of their stale Feature Attribute Matrix.
 *
 * The adapter changes only copy actions. The crop filter retains its parameter checks, bounds cache and execution path.
 */
class SIMPLNXCORE_EXPORT ExtractionCropFilter : public CropImageGeometryFilter
{
public:
  /**
   * @brief Selects the source Feature Attribute Matrix to exclude from recursive copies.
   * @param excludedPath Identifies the matrix by any source link.
   */
  explicit ExtractionCropFilter(DataPath excludedPath)
  : m_ExcludedPath(std::move(excludedPath))
  {
  }
  /** @brief Releases the adapter's saved path. */
  ~ExtractionCropFilter() noexcept override = default;
  ExtractionCropFilter(const ExtractionCropFilter&) = delete;
  ExtractionCropFilter(ExtractionCropFilter&&) = delete;
  ExtractionCropFilter& operator=(const ExtractionCropFilter&) = delete;
  ExtractionCropFilter& operator=(ExtractionCropFilter&&) = delete;

protected:
  /**
   * @brief Replaces crop copy actions with copies that exclude the selected Feature Attribute Matrix.
   * @param dataStructure Supplies the source objects and their links.
   * @param args Supplies the crop parameters.
   * @param messageHandler Receives crop preflight messages.
   * @param shouldCancel Stops crop preflight when requested.
   * @param executionContext Supplies the crop execution context.
   * @return The crop actions with Feature data excluded, or the original preflight error.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override
  {
    auto result = CropImageGeometryFilter::preflightImpl(dataStructure, args, messageHandler, shouldCancel, executionContext);
    if(result.outputActions.invalid())
    {
      return result;
    }
    OutputActions copies;
    for(auto& action : result.outputActions.value().actions)
    {
      if(const auto* copy = dynamic_cast<const CopyDataObjectAction*>(action.get()); copy != nullptr)
      {
        AppendCopyDataObjectAction(dataStructure, copies, copy->path(), copy->newPath(), m_ExcludedPath);
      }
      else
      {
        copies.actions.push_back(std::move(action));
      }
    }
    result.outputActions.value().actions = std::move(copies.actions);
    return result;
  }

private:
  DataPath m_ExcludedPath;
};

class RunCropImageGeometryImpl
{
public:
  /**
   * @brief Creates one borrowed crop task.
   * @param dataStructure Receives the cropped geometry.
   * @param shouldCancel Stops before delegated execution when true.
   * @param imageGeometryPath Identifies the source ImageGeom.
   * @param minVoxelVector Specifies inclusive minimum voxel indexes.
   * @param maxVoxelVector Specifies inclusive maximum voxel indexes.
   * @param createdImgGeomPath Identifies the cropped ImageGeom.
   * @param featureAttributeMatrixName Identifies the Feature Attribute Matrix to exclude from the crop.
   * @param taskResult Receives the delegated preflight or execute failure.
   */
  RunCropImageGeometryImpl(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const DataPath& imageGeometryPath, const std::vector<uint64>& minVoxelVector,
                           const std::vector<uint64>& maxVoxelVector, const DataPath& createdImgGeomPath, const std::string& featureAttributeMatrixName, CopyFromArray::ParallelTaskResult& taskResult)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_ImageGeometryPath(imageGeometryPath)
  , m_MinVoxelVector(minVoxelVector)
  , m_MaxVoxelVector(maxVoxelVector)
  , m_CreatedImgGeomPath(createdImgGeomPath)
  , m_FeatureAttributeMatrixName(featureAttributeMatrixName)
  , m_TaskResult(taskResult)
  {
  }

  /**
   * @brief Destroys the borrowed crop task.
   */
  ~RunCropImageGeometryImpl() = default;

  /**
   * @brief Preflights and executes the delegated crop, latching any failure.
   *
   * ParallelTaskAlgorithm tasks cannot return a value, so the delegated filter's
   * preflight and execute errors travel through the shared first-error holder and
   * stop the caller before it reports success.
   */
  void operator()() const
  {
    ExtractionCropFilter filter(m_ImageGeometryPath.createChildPath(m_FeatureAttributeMatrixName));

    Arguments args;

    args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(m_ImageGeometryPath));
    // Exclude the Feature AM from copy actions. Clear=false avoids creating an empty replacement.
    args.insertOrAssign(CropImageGeometryFilter::k_ClearFeatureAttributeMatrix_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

    args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(m_MinVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(m_MaxVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(m_CreatedImgGeomPath));

    const std::string boundsText =
        fmt::format("voxels [{}, {}, {}] to [{}, {}, {}]", m_MinVoxelVector[0], m_MinVoxelVector[1], m_MinVoxelVector[2], m_MaxVoxelVector[0], m_MaxVoxelVector[1], m_MaxVoxelVector[2]);

    auto preflightResult = filter.preflight(m_DataStructure, args);
    if(preflightResult.outputActions.invalid())
    {
      m_TaskResult.store(MakeErrorResult(-53903, fmt::format("Preflight of the crop that extracts feature geometry '{}' ({}) from '{}' failed: {}", m_CreatedImgGeomPath.toString(), boundsText,
                                                             m_ImageGeometryPath.toString(), FirstRemoveFlaggedFeaturesErrorMessage(preflightResult.outputActions.errors()))));
      return;
    }

    if(m_ShouldCancel)
    {
      return;
    }

    auto executeResult = filter.execute(m_DataStructure, args);
    if(executeResult.result.invalid())
    {
      m_TaskResult.store(MakeErrorResult(-53904, fmt::format("The crop that extracts feature geometry '{}' ({}) from '{}' failed: {}", m_CreatedImgGeomPath.toString(), boundsText,
                                                             m_ImageGeometryPath.toString(), FirstRemoveFlaggedFeaturesErrorMessage(executeResult.result.errors()))));
      return;
    }
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const DataPath& m_ImageGeometryPath;
  const std::vector<uint64>& m_MinVoxelVector;
  const std::vector<uint64>& m_MaxVoxelVector;
  const DataPath& m_CreatedImgGeomPath;
  const std::string m_FeatureAttributeMatrixName;
  CopyFromArray::ParallelTaskResult& m_TaskResult;
};

} // namespace nx::core
