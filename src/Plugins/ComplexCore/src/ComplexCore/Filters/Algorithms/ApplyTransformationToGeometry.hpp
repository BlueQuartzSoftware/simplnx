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

struct COMPLEXCORE_EXPORT ApplyTransformationToGeometryInputValues
{
  DataPath pGeometryToTransform;
  TransformType pTransformationType;
  std::vector<float> transformationMatrix;
};

class COMPLEXCORE_EXPORT ApplyTransformationToGeometry
{
public:
  ApplyTransformationToGeometry(DataStructure& data, ApplyTransformationToGeometryInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ApplyTransformationToGeometry() noexcept;

  ApplyTransformationToGeometry(const ApplyTransformationToGeometry&) = delete;
  ApplyTransformationToGeometry(ApplyTransformationToGeometry&&) noexcept = delete;
  ApplyTransformationToGeometry& operator=(const ApplyTransformationToGeometry&) = delete;
  ApplyTransformationToGeometry& operator=(ApplyTransformationToGeometry&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Allows thread safe progress updates
   * @param counter
   */
  void sendThreadSafeProgressMessage(int64_t counter);

private:
  DataStructure& m_DataStructure;
  const ApplyTransformationToGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_InstanceIndex = {0};
  size_t m_TotalElements = {0};

  /**
   * @brief Applies the 4x4 Transform matrix to the node based geometry
   */
  Result<> applyTransformation();
};

} // namespace complex
