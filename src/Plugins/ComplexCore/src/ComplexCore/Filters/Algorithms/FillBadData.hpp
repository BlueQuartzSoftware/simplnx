
#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

#include <vector>

namespace complex
{

struct COMPLEXCORE_EXPORT FillBadDataInputValues
{
  int32 minAllowedDefectSizeValue;
  bool storeAsNewPhase;
  DataPath featureIdsArrayPath;
  DataPath cellPhasesArrayPath;
  std::vector<DataPath> ignoredDataArrayPaths;
  DataPath inputImageGeometry;
};

/**
 * @class FillBadData
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */
class COMPLEXCORE_EXPORT FillBadData
{
public:
  FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues);
  ~FillBadData() noexcept;

  FillBadData(const FillBadData&) = delete;
  FillBadData(FillBadData&&) noexcept = delete;
  FillBadData& operator=(const FillBadData&) = delete;
  FillBadData& operator=(FillBadData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
