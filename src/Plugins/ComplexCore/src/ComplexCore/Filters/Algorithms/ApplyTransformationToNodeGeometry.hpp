#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace complex
{

enum class TransformType
{
  No_Transform = 0,
  PreComputed_TransformMatrix,
  ManualTransformMatrix,
  Rotation,
  Translation,
  Scale
};

struct COMPLEXCORE_EXPORT ApplyTransformationToNodeGeometryInputValues
{
  DataPath pGeometryToTransform;
  TransformType pTransformationType;
  std::vector<float> transformationMatrix;
};

class COMPLEXCORE_EXPORT ApplyTransformationToNodeGeometry
{
public:
  ApplyTransformationToNodeGeometry(DataStructure& data, ApplyTransformationToNodeGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ApplyTransformationToNodeGeometry() noexcept;

  ApplyTransformationToNodeGeometry(const ApplyTransformationToNodeGeometry&) = delete;
  ApplyTransformationToNodeGeometry(ApplyTransformationToNodeGeometry&&) noexcept = delete;
  ApplyTransformationToNodeGeometry& operator=(const ApplyTransformationToNodeGeometry&) = delete;
  ApplyTransformationToNodeGeometry& operator=(ApplyTransformationToNodeGeometry&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Allows thread safe progress updates
   * @param counter
   */
  void sendThreadSafeProgressMessage(size_t counter);

private:
  DataStructure& m_DataStructure;
  const ApplyTransformationToNodeGeometryInputValues* m_InputValues = nullptr;
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
