#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindVertexToTriangleDistancesInputValues
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

class SIMPLNXCORE_EXPORT FindVertexToTriangleDistances
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

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
};

} // namespace nx::core
