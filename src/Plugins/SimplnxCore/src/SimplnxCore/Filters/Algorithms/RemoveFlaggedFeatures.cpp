#include "RemoveFlaggedFeatures.hpp"

#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"
#include "SimplnxCore/utils/FeatureRemovalUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_MaskArrayError = -53900;
constexpr int32 k_FeatureRectPreflightError = -53901;
constexpr int32 k_FeatureRectExecuteError = -53902;
constexpr int32 k_CropPreflightError = -53903;
constexpr int32 k_CropExecuteError = -53904;
constexpr int32 k_EmptyFeatureSkippedWarning = -53905;

/// Number of empty feature ids listed in the -53905 warning before the list is truncated.
constexpr usize k_MaxListedEmptyFeatures = 10;

/**
 * @brief Formats the first error of a sub-filter result for embedding in this filter's message.
 * @param errors Errors returned by the sub-filter.
 * @return "[code] message", with a count of any further errors appended.
 */
std::string FirstErrorMessage(const std::vector<Error>& errors)
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
 * @brief Crops one feature's bounding box out of the source Image Geometry into a new geometry.
 *
 * @param dataStructure Receives the new geometry.
 * @param shouldCancel Checked between the crop preflight and execute.
 * @param imageGeometryPath The source Image Geometry.
 * @param minVoxels Inclusive minimum (x, y, z) voxel indices of the bounding box.
 * @param maxVoxels Inclusive maximum (x, y, z) voxel indices of the bounding box.
 * @param createdImgGeomPath Path of the new geometry.
 * @return Error -53903 if the crop preflight fails, -53904 if its execute fails, otherwise valid.
 */
Result<> CropFeature(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const DataPath& imageGeometryPath, const std::vector<uint64>& minVoxels, const std::vector<uint64>& maxVoxels,
                     const DataPath& createdImgGeomPath)
{
  CropImageGeometryFilter filter;

  Arguments args;

  args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeometryPath));
  args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

  args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(minVoxels));
  args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(maxVoxels));
  args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(createdImgGeomPath));

  const std::string boundsText = fmt::format("voxels [{}, {}, {}] to [{}, {}, {}]", minVoxels[0], minVoxels[1], minVoxels[2], maxVoxels[0], maxVoxels[1], maxVoxels[2]);

  auto preflightResult = filter.preflight(dataStructure, args);
  if(preflightResult.outputActions.invalid())
  {
    return MakeErrorResult(k_CropPreflightError, fmt::format("Preflight of the crop that extracts feature geometry '{}' ({}) from '{}' failed: {}", createdImgGeomPath.toString(), boundsText,
                                                             imageGeometryPath.toString(), FirstErrorMessage(preflightResult.outputActions.errors())));
  }

  if(shouldCancel)
  {
    return {};
  }

  auto executeResult = filter.execute(dataStructure, args);
  if(executeResult.result.invalid())
  {
    return MakeErrorResult(k_CropExecuteError, fmt::format("The crop that extracts feature geometry '{}' ({}) from '{}' failed: {}", createdImgGeomPath.toString(), boundsText,
                                                           imageGeometryPath.toString(), FirstErrorMessage(executeResult.result.errors())));
  }
  return {};
}
} // namespace

// -----------------------------------------------------------------------------
RemoveFlaggedFeatures::RemoveFlaggedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             RemoveFlaggedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RemoveFlaggedFeatures::~RemoveFlaggedFeatures() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RemoveFlaggedFeatures::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RemoveFlaggedFeatures::operator()()
{
  auto function = static_cast<Functionality>(m_InputValues->ExtractFeatures);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> flaggedFeatures = nullptr;
  try
  {
    flaggedFeatures = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->FlaggedFeaturesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // Preflight verifies the path, but the algorithm can also be run directly without preflight.
    return MakeErrorResult(k_MaskArrayError,
                           fmt::format("The flagged features array at path '{}' does not exist or is not of type Bool or UInt8.", m_InputValues->FlaggedFeaturesArrayPath.toString()));
  }

  if(getCancel())
  {
    return {};
  }

  Result<> extractWarnings;

  // Valid values Functionality::Extract and Functionality::ExtractThenRemove
  if(function != Functionality::Remove)
  {
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Extraction")});

    {
      ComputeFeatureRectFilter filter;
      Arguments args;

      args.insert(ComputeFeatureRectFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(m_InputValues->FeatureIdsArrayPath));
      args.insert(ComputeFeatureRectFilter::k_FeatureDataAttributeMatrixPath_Key, std::make_any<DataPath>(m_InputValues->TempBoundsPath.getParent()));
      args.insert(ComputeFeatureRectFilter::k_FeatureRectArrayName_Key, std::make_any<std::string>(m_InputValues->TempBoundsPath.getTargetName()));

      auto preflightResult = filter.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        return MakeErrorResult(k_FeatureRectPreflightError, fmt::format("Preflight of the feature bounding-box computation for Feature Ids array '{}' failed: {}",
                                                                        m_InputValues->FeatureIdsArrayPath.toString(), FirstErrorMessage(preflightResult.outputActions.errors())));
      }

      if(getCancel())
      {
        return {};
      }

      auto executeResult = filter.execute(m_DataStructure, args);
      if(executeResult.result.invalid())
      {
        m_DataStructure.removeData(m_InputValues->TempBoundsPath);
        return MakeErrorResult(k_FeatureRectExecuteError, fmt::format("The feature bounding-box computation for Feature Ids array '{}' failed: {}", m_InputValues->FeatureIdsArrayPath.toString(),
                                                                      FirstErrorMessage(executeResult.result.errors())));
      }
    }

    // Copy the six bounds per feature out of the temporary array, then delete it at once. Deleting
    // it here keeps it out of every extracted geometry (the crop copies the whole feature Attribute
    // Matrix) and out of the DataStructure on every later error return.
    std::vector<uint32> bounds;
    {
      const auto& boundsStore = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->TempBoundsPath).getDataStoreRef();
      bounds.resize(boundsStore.getSize());
      for(usize i = 0; i < bounds.size(); i++)
      {
        bounds[i] = boundsStore[i];
      }
    }
    m_DataStructure.removeData(m_InputValues->TempBoundsPath);

    if(getCancel())
    {
      return {};
    }

    // Each crop adds a geometry to the DataStructure, which is not thread-safe, so the features are
    // extracted one at a time.
    const usize maxTuple = flaggedFeatures->getNumberOfTuples();
    const std::string paddingWidth = std::to_string(std::to_string(maxTuple).size());
    std::vector<usize> emptyFeatures;
    for(usize i = 1; i < maxTuple && 6 * i + 5 < bounds.size(); i++)
    {
      if(getCancel())
      {
        return {};
      }

      if(!flaggedFeatures->isTrue(i))
      {
        continue;
      }

      const usize index = 6 * i;
      std::vector<uint64> minVoxels = {static_cast<uint64>(bounds[index]), static_cast<uint64>(bounds[index + 1]), static_cast<uint64>(bounds[index + 2])};
      std::vector<uint64> maxVoxels = {static_cast<uint64>(bounds[index + 3]), static_cast<uint64>(bounds[index + 4]), static_cast<uint64>(bounds[index + 5])};

      // ComputeFeatureRect initializes each minimum to the largest uint32 and each maximum to 0, so a
      // feature that owns no cell is left with minimum > maximum. There is nothing to crop for it.
      if(minVoxels[0] > maxVoxels[0] || minVoxels[1] > maxVoxels[1] || minVoxels[2] > maxVoxels[2])
      {
        emptyFeatures.push_back(i);
        continue;
      }

      DataPath createdImgGeomPath({fmt::format(fmt::runtime("{}-{:0" + paddingWidth + "d}"), m_InputValues->CreatedImageGeometryPrefix, i)});

      m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Now Extracting Feature {}", i)});
      Result<> cropResult = CropFeature(m_DataStructure, getCancel(), m_InputValues->ImageGeometryPath, minVoxels, maxVoxels, createdImgGeomPath);
      if(cropResult.invalid())
      {
        return MergeResults(std::move(extractWarnings), std::move(cropResult));
      }
    }

    if(!emptyFeatures.empty())
    {
      std::string listed;
      for(usize k = 0; k < std::min(emptyFeatures.size(), k_MaxListedEmptyFeatures); k++)
      {
        listed += fmt::format("{}{}", k == 0 ? "" : ", ", emptyFeatures[k]);
      }
      if(emptyFeatures.size() > k_MaxListedEmptyFeatures)
      {
        listed += ", ...";
      }
      extractWarnings.warnings().push_back(Warning{k_EmptyFeatureSkippedWarning, fmt::format("{} flagged feature(s) own no cell in the Feature Ids array '{}' and were skipped; no geometry was "
                                                                                             "created for them. Feature id(s): {}",
                                                                                             emptyFeatures.size(), m_InputValues->FeatureIdsArrayPath.toString(), listed)});
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("All Features Successfully Extracted")});
  }

  if(getCancel())
  {
    return {};
  }

  // Valid values Functionality::Remove and Functionality::ExtractThenRemove
  if(function != Functionality::Extract)
  {
    // Adapt the MaskCompare into a plain flag vector for the shared removal routine.
    const usize totalFeatures = flaggedFeatures->getNumberOfTuples();
    std::vector<bool> flagVector(totalFeatures, false);
    for(usize i = 1; i < totalFeatures; i++)
    {
      flagVector[i] = flaggedFeatures->isTrue(i);
    }

    FeatureRemovalUtilities::RemovalArgs removalArgs;
    removalArgs.ImageGeometryPath = m_InputValues->ImageGeometryPath;
    removalArgs.FeatureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;
    removalArgs.FeatureAttributeMatrixPath = m_InputValues->FlaggedFeaturesArrayPath.getParent();
    removalArgs.IgnoredDataArrayPaths = m_InputValues->IgnoredDataArrayPaths;
    removalArgs.FillRemovedFeatures = m_InputValues->FillRemovedFeatures;

    Result<> removeResult = FeatureRemovalUtilities::removeFlaggedFeatures(m_DataStructure, flagVector, removalArgs, m_MessageHandler, m_ShouldCancel);
    return MergeResults(std::move(extractWarnings), std::move(removeResult));
  }

  return extractWarnings;
}
