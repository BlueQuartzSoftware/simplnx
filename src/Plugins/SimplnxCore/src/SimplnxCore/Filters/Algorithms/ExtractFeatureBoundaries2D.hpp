#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ExtractFeatureBoundaries2DInputValues
 * @brief Collects boundary, placement, and output settings.
 */
struct SIMPLNXCORE_EXPORT ExtractFeatureBoundaries2DInputValues
{
  /**
   * @enum ZValueChoiceType
   * @brief Selects the common Z coordinate for output vertices.
   */
  enum class ZValueChoiceType : uint64
  {
    UseMinZValue = 0,   // Uses the ImageGeom origin.
    UseMaxZValue = 1,   // Uses the upper physical ImageGeom bound.
    UseCustomZValue = 2 // Uses CustomZValue.
  };

  DataPath InputImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath OutputEdgeGeometryPath;
  ZValueChoiceType ZValueChoice = ZValueChoiceType::UseMinZValue;
  float32 CustomZValue = 0.0f;
  bool ExtractVirtualSampleEdges = false;
};

/**
 * @class ExtractFeatureBoundaries2D
 * @brief Creates an EdgeGeom for boundaries in a one-slice ImageGeom.
 *
 * The algorithm reads Feature IDs with two row buffers. It first counts edges,
 * then allocates exact edge storage and populates it. Each edge initially owns
 * two vertices. A final deduplication merges coincident endpoints.
 *
 * Input scratch scales with image width. Output storage scales with extracted
 * edges and uses direct sequential writes.
 */
class SIMPLNXCORE_EXPORT ExtractFeatureBoundaries2D
{
public:
  /**
   * @brief Initializes 2D boundary extraction.
   * @param dataStructure Contains input and output geometry.
   * @param mesgHandler Receives the extraction phase message.
   * @param shouldCancel Signals cancellation during row passes.
   * @param inputValues Selects input, placement, perimeter, and output settings.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ExtractFeatureBoundaries2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractFeatureBoundaries2DInputValues* inputValues);
  ~ExtractFeatureBoundaries2D() noexcept;

  ExtractFeatureBoundaries2D(const ExtractFeatureBoundaries2D&) = delete;
  ExtractFeatureBoundaries2D(ExtractFeatureBoundaries2D&&) noexcept = delete;
  ExtractFeatureBoundaries2D& operator=(const ExtractFeatureBoundaries2D&) = delete;
  ExtractFeatureBoundaries2D& operator=(ExtractFeatureBoundaries2D&&) noexcept = delete;

  /**
   * @brief Extracts internal boundaries and an optional sample perimeter.
   * @return Success, or an input-read or vertex-deduplication error.
   * @pre The ImageGeom has one nonempty Z slice and matches Feature ID tuples.
   * @pre Edge and initial vertex counts fit usize.
   *
   * Cancellation during counting leaves preflight output unchanged. Cancellation
   * during population returns success with allocated and partially written
   * geometry. Perimeter creation and vertex deduplication do not check
   * cancellation.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel() const;

private:
  DataStructure& m_DataStructure;
  const ExtractFeatureBoundaries2DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
