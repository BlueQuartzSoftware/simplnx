#pragma once

#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>

#include <any>
#include <atomic>
#include <utility>
#include <vector>

namespace nx::core
{

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
   * @param taskResult Receives the delegated preflight or execute failure.
   */
  RunCropImageGeometryImpl(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const DataPath& imageGeometryPath, const std::vector<uint64>& minVoxelVector,
                           const std::vector<uint64>& maxVoxelVector, const DataPath& createdImgGeomPath, CopyFromArray::ParallelTaskResult& taskResult)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_ImageGeometryPath(imageGeometryPath)
  , m_MinVoxelVector(minVoxelVector)
  , m_MaxVoxelVector(maxVoxelVector)
  , m_CreatedImgGeomPath(createdImgGeomPath)
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
    CropImageGeometryFilter filter;

    Arguments args;

    args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(m_ImageGeometryPath));
    args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

    args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(m_MinVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(m_MaxVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(m_CreatedImgGeomPath));

    auto preflightResult = filter.preflight(m_DataStructure, args);
    if(preflightResult.outputActions.invalid())
    {
      // The delegated filter reports the real cause, so its own errors are kept and each one is
      // prefixed with this call's context instead of being buried behind a second error object.
      Result<> delegatedResult = ConvertResult(std::move(preflightResult.outputActions));
      if(delegatedResult.valid() || delegatedResult.errors().empty())
      {
        // Defensive: the delegated preflight reported a failure without any error to explain it.
        m_TaskResult.store(MakeErrorResult(-45440, fmt::format("RemoveFlaggedFeatures: cropping '{}' from '{}' failed: the delegated crop preflight reported a failure without a cause.",
                                                               m_CreatedImgGeomPath.toString(), m_ImageGeometryPath.toString())));
        return;
      }
      for(Error& error : delegatedResult.errors())
      {
        error.message = fmt::format("RemoveFlaggedFeatures: cropping '{}' from '{}' failed: {}", m_CreatedImgGeomPath.toString(), m_ImageGeometryPath.toString(), error.message);
      }
      m_TaskResult.store(std::move(delegatedResult));
      return;
    }

    if(m_ShouldCancel)
    {
      return;
    }

    auto executeResult = filter.execute(m_DataStructure, args);
    if(executeResult.result.invalid())
    {
      // The delegated filter reports the real cause, so its own errors are kept and each one is
      // prefixed with this call's context instead of being buried behind a second error object.
      Result<> delegatedResult = std::move(executeResult.result);
      if(delegatedResult.errors().empty())
      {
        // Defensive: the delegated execution reported a failure without any error to explain it.
        m_TaskResult.store(MakeErrorResult(-45441, fmt::format("RemoveFlaggedFeatures: cropping '{}' from '{}' failed: the delegated crop execution reported a failure without a cause.",
                                                               m_CreatedImgGeomPath.toString(), m_ImageGeometryPath.toString())));
        return;
      }
      for(Error& error : delegatedResult.errors())
      {
        error.message = fmt::format("RemoveFlaggedFeatures: cropping '{}' from '{}' failed: {}", m_CreatedImgGeomPath.toString(), m_ImageGeometryPath.toString(), error.message);
      }
      m_TaskResult.store(std::move(delegatedResult));
    }
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const DataPath& m_ImageGeometryPath;
  const std::vector<uint64>& m_MinVoxelVector;
  const std::vector<uint64>& m_MaxVoxelVector;
  const DataPath& m_CreatedImgGeomPath;
  CopyFromArray::ParallelTaskResult& m_TaskResult;
};

} // namespace nx::core
