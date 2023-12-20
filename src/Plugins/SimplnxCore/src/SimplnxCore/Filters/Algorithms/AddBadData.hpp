#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT AddBadData
{
public:
  AddBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AddBadDataInputValues* inputValues);
  ~AddBadData() noexcept;

  AddBadData(const AddBadData&) = delete;
  AddBadData(AddBadData&&) noexcept = delete;
  AddBadData& operator=(const AddBadData&) = delete;
  AddBadData& operator=(AddBadData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AddBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
