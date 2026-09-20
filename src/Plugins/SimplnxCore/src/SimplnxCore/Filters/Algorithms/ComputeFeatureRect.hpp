#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeFeatureRectInputValues
 * @brief Stores filter values for feature-rectangle calculation.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureRectInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureDataAttributeMatrixPath;
  DataPath FeatureRectArrayPath;
};

/**
 * @class ComputeFeatureRect
 * @brief Computes six X, Y, and Z index bounds for each non-background feature.
 *
 * Feature IDs use 65,536-value bulk reads. The algorithm retains six uint32
 * extrema per feature and writes them in one bulk transfer. Feature zero is
 * background and has no rectangle output.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureRect
{
public:
  /**
   * @brief Initializes the feature-rectangle algorithm.
   * @param dataStructure Contains Feature IDs and rectangle output.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between cell chunks.
   * @param inputValues Identifies required arrays and output path.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureRect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureRectInputValues* inputValues);
  /**
   * @brief Destroys the feature-rectangle algorithm.
   */
  ~ComputeFeatureRect() noexcept;

  ComputeFeatureRect(const ComputeFeatureRect&) = delete;
  ComputeFeatureRect(ComputeFeatureRect&&) noexcept = delete;
  ComputeFeatureRect& operator=(const ComputeFeatureRect&) = delete;
  ComputeFeatureRect& operator=(ComputeFeatureRect&&) noexcept = delete;

  /**
   * @brief Computes feature index rectangles.
   * @return Success, or a Feature ID read, rectangle-write, or invalid-ID error.
   *
   * Cancellation writes extrema accumulated before its checkpoint. An invalid
   * Feature ID also writes completed extrema before the method returns its error.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureRectInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
