#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT CreateAMScanPathsInputValues
{
  float32 StripeWidth;
  float32 HatchSpacing;
  float32 SliceHatchRotationAngle;
  DataPath CADSliceDataContainerName;
  DataPath CADSliceIdsArrayPath;
  DataPath CADRegionIdsArrayPath;
  DataPath HatchDataContainerName;
  DataObjectNameParameter::ValueType VertexAttributeMatrixName;
  DataObjectNameParameter::ValueType HatchAttributeMatrixName;
  DataObjectNameParameter::ValueType RegionIdsArrayName;
};

namespace CreateAMScanPathsDetail
{
/**
 * @brief Checks that a hatch Y coordinate fits in the output coordinate type.
 * @param position Gives the Y coordinate in the rotated frame.
 * @param isStart Selects the diagnostic for the first line before counting.
 * @param inputPath Identifies the input edge geometry.
 * @return The float32 coordinate or an error.
 */
SIMPLNXCORE_EXPORT Result<float32> CheckedHatchY(float64 position, bool isStart, const DataPath& inputPath);

/**
 * @brief Orders intersection X coordinates with NaN after all numbers.
 * @param left Gives the first X coordinate.
 * @param right Gives the second X coordinate.
 * @return True if the first coordinate precedes the second.
 */
SIMPLNXCORE_EXPORT bool IntersectionXLess(float32 left, float32 right);
} // namespace CreateAMScanPathsDetail

/**
 * @class CreateAMScanPaths
 * @brief This filter will generate additive manufacturing scan paths from an edge geometry
 */

class SIMPLNXCORE_EXPORT CreateAMScanPaths
{
public:
  CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues);
  ~CreateAMScanPaths() noexcept;

  CreateAMScanPaths(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths(CreateAMScanPaths&&) noexcept = delete;
  CreateAMScanPaths& operator=(const CreateAMScanPaths&) = delete;
  CreateAMScanPaths& operator=(CreateAMScanPaths&&) noexcept = delete;

  /**
   * @brief Generates hatch paths from closed slice edges.
   * @pre Hatch Spacing is finite and positive; consumed rotation radians are finite.
   * @return Success, a hatch position or count error, or an output resize error.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateAMScanPathsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
