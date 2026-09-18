#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SampleSurfaceMesh.hpp"

#include <random>

namespace nx::core
{
/**
 * @struct UncertainRegularGridSampleSurfaceMeshInputValues
 * @brief Stores grid, uncertainty, seed, mesh, label, and output selections.
 */
struct SIMPLNXCORE_EXPORT UncertainRegularGridSampleSurfaceMeshInputValues
{
  uint64 SeedValue;
  VectorUInt64Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Uncertainty;
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class UncertainRegularGridSampleSurfaceMesh
 * @brief Samples a triangle mesh on a jittered regular grid.
 *
 * Each coordinate starts at a grid-cell center. A uniform random value in
 * [-1, 1) multiplies the selected axis uncertainty. One Z draw applies to a
 * complete slice. One Y draw applies to a complete row. Each point gets one X
 * draw. Increasing-Z generation preserves the monolithic full-volume sequence.
 *
 * The pseudo-random generator is seeded once at construction and is not reset
 * by operator(). Reusing one algorithm object continues the prior sequence.
 * std::mt19937 uses its engine seed width, so higher SeedValue bits can be lost.
 *
 * SampleSurfaceMesh retains one point/output XY slice and mesh-scale face lookup
 * data. Point-in-polyhedron tests run in parallel and read shared geometry stores.
 * Generic DataStore implementations do not guarantee those concurrent reads.
 */
class SIMPLNXCORE_EXPORT UncertainRegularGridSampleSurfaceMesh : public SampleSurfaceMesh
{
public:
  /**
   * @brief Initializes the uncertain regular-grid sampler.
   * @param dataStructure Contains mesh and output data.
   * @param mesgHandler Receives sampling progress messages.
   * @param shouldCancel Signals cancellation between generation and point tests.
   * @param inputValues Selects grid, uncertainty, seed, and paths.
   * @pre inputValues is not null.
   * @pre All arguments outlive this sampler.
   */
  UncertainRegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        UncertainRegularGridSampleSurfaceMeshInputValues* inputValues);
  /**
   * @brief Destroys the uncertain regular-grid sampler.
   */
  ~UncertainRegularGridSampleSurfaceMesh() noexcept override;

  UncertainRegularGridSampleSurfaceMesh(const UncertainRegularGridSampleSurfaceMesh&) = delete;
  UncertainRegularGridSampleSurfaceMesh(UncertainRegularGridSampleSurfaceMesh&&) noexcept = delete;
  UncertainRegularGridSampleSurfaceMesh& operator=(const UncertainRegularGridSampleSurfaceMesh&) = delete;
  UncertainRegularGridSampleSurfaceMesh& operator=(UncertainRegularGridSampleSurfaceMesh&&) noexcept = delete;

  /**
   * @brief Generates jittered slices and assigns enclosing feature IDs.
   * @return Output bulk-write or feature-ID overflow result.
   * @pre Dimensions fit usize and their X-Y product fits usize.
   * @pre Spacing and uncertainty contain X, Y, and Z values.
   *
   * Cancellation returns success. A slice is written only after its generation
   * and point tests complete. Prior slices remain in output.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  /**
   * @brief Gets grid dimensions in X, Y, and Z order.
   * @return Dimensions converted to usize.
   * @pre Each configured dimension fits usize.
   */
  SizeVec3 getGridDimensions() const override;

  /**
   * @brief Generates one slice of jittered cell-center points.
   * @param zSlice Z-slice index.
   * @param slicePoints Receives X-fastest points.
   * @pre zSlice is in range and slicePoints has X times Y elements.
   *
   * This method does not inspect cancellation during the slice draw sequence.
   */
  void generateSlicePoints(usize zSlice, std::vector<Point3Df>& slicePoints) override;

private:
  DataStructure& m_DataStructure;
  const UncertainRegularGridSampleSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Persistent state preserves the draw sequence across increasing Z slices.
  std::mt19937 m_Generator;
  std::uniform_real_distribution<float32> m_Distribution{0.0F, 1.0F};
};
} // namespace nx::core
