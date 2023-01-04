#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindVertexToTriangleDistancesInputValues
{
  DataPath VertexDataContainer;
  DataPath TriangleDataContainer;
  DataPath TriangleNormalsArrayPath;
  DataPath DistancesArrayPath;
  DataPath ClosestTriangleIdArrayPath;
  DataPath TriBoundsDataPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindVertexToTriangleDistances
{
public:
  FindVertexToTriangleDistances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindVertexToTriangleDistancesInputValues* inputValues);
  ~FindVertexToTriangleDistances() noexcept;

  FindVertexToTriangleDistances(const FindVertexToTriangleDistances&) = delete;
  FindVertexToTriangleDistances(FindVertexToTriangleDistances&&) noexcept = delete;
  FindVertexToTriangleDistances& operator=(const FindVertexToTriangleDistances&) = delete;
  FindVertexToTriangleDistances& operator=(FindVertexToTriangleDistances&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const FindVertexToTriangleDistancesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
};

} // namespace complex
