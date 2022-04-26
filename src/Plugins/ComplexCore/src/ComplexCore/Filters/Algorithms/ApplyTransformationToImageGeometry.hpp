#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/Filters/Algorithms/ApplyTransformationToNodeGeometry.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace complex
{
struct COMPLEXCORE_EXPORT ApplyTransformationToImageGeometryInputValues
{
  DataPath pGeometryToTransform;
  TransformType pTransformationType;
  std::vector<float> transformationMatrix;
};

class COMPLEXCORE_EXPORT ApplyTransformationToImageGeometry
{
public:
  ApplyTransformationToImageGeometry(DataStructure& data, ApplyTransformationToImageGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ApplyTransformationToImageGeometry() noexcept;

  ApplyTransformationToImageGeometry(const ApplyTransformationToImageGeometry&) = delete;
  ApplyTransformationToImageGeometry(ApplyTransformationToImageGeometry&&) noexcept = delete;
  ApplyTransformationToImageGeometry& operator=(const ApplyTransformationToImageGeometry&) = delete;
  ApplyTransformationToImageGeometry& operator=(ApplyTransformationToImageGeometry&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Allows thread safe progress updates
   * @param counter
   */
  void sendThreadSafeProgressMessage(size_t counter);

private:
  DataStructure& m_DataStructure;
  const ApplyTransformationToImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_InstanceIndex = 0;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
};

} // namespace complex
