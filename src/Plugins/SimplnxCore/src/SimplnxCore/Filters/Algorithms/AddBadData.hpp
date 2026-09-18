#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

/**
 * @struct AddBadDataInputValues
 * @brief Defines seeded boundary and Poisson noise settings and source paths.
 */
struct SIMPLNXCORE_EXPORT AddBadDataInputValues
{
  bool PoissonNoise;
  float32 PoissonVolFraction;
  bool BoundaryNoise;
  float32 BoundaryVolFraction;
  DataPath GBEuclideanDistancesArrayPath;
  DataPath ImageGeometryPath;
  uint64 SeedValue;
};

/**
 * @class AddBadData
 * @brief Adds seeded boundary and Poisson bad data by zeroing selected cell tuples.
 *
 * The algorithm applies each selected tuple to every numeric cell array in the
 * image geometry. Boundary and Poisson selection use independent random draws.
 * Direct and scanline paths preserve the same draw order for a given seed.
 *
 * Concrete in-memory stores use direct pointers. An out-of-core child selects
 * 65,536-tuple bulk-I/O pages. Staging bytes also scale with the largest child
 * component count. Cancellation returns success with prior tuple mutations
 * preserved. A transfer error can leave earlier child arrays changed.
 */
class SIMPLNXCORE_EXPORT AddBadData
{
public:
  /**
   * @brief Initializes seeded bad-data mutation.
   * @param dataStructure Provides the image and cell arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between pages.
   * @param inputValues Defines noise modes, fractions, paths, and seed.
   * @pre All arguments outlive this executor.
   */
  AddBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AddBadDataInputValues* inputValues);
  ~AddBadData() noexcept;

  AddBadData(const AddBadData&) = delete;
  AddBadData(AddBadData&&) noexcept = delete;
  AddBadData& operator=(const AddBadData&) = delete;
  AddBadData& operator=(AddBadData&&) noexcept = delete;

  /**
   * @brief Zeros selected tuples in all numeric cell arrays.
   * @return Source and destination transfer errors.
   * @pre Noise fractions are in the range [0, 1].
   * @pre The distance array and all child arrays match the image cell count.
   * @pre Direct-path in-memory stores have concrete DataStore dynamic types.
   *
   * The current implementation treats child-array discovery failure as an empty
   * child list and returns success.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AddBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
