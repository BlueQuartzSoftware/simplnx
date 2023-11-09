#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

namespace complex
{

struct COMPLEXCORE_EXPORT InterpolateValuesToUnstructuredGridInputValues
{
  DataPath SourceGeomPath;
  DataPath DestinationGeomPath;
  std::vector<DataPath> InputDataPaths;
  bool UseExistingAttrMatrix;
  DataPath ExistingAttrMatrixPath;
  std::string CreatedAttrMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT InterpolateValuesToUnstructuredGrid
{
public:
  InterpolateValuesToUnstructuredGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                      InterpolateValuesToUnstructuredGridInputValues* inputValues);
  ~InterpolateValuesToUnstructuredGrid() noexcept;

  InterpolateValuesToUnstructuredGrid(const InterpolateValuesToUnstructuredGrid&) = delete;
  InterpolateValuesToUnstructuredGrid(InterpolateValuesToUnstructuredGrid&&) noexcept = delete;
  InterpolateValuesToUnstructuredGrid& operator=(const InterpolateValuesToUnstructuredGrid&) = delete;
  InterpolateValuesToUnstructuredGrid& operator=(InterpolateValuesToUnstructuredGrid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const InterpolateValuesToUnstructuredGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  std::chrono::steady_clock::time_point m_InitialPoint = std::chrono::steady_clock::now();
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
};

} // namespace complex
