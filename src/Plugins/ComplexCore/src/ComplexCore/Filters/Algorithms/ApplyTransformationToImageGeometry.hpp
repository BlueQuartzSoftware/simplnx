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
struct RotateArgs
{
  int64 xp = 0;
  int64 yp = 0;
  int64 zp = 0;
  float32 xRes = 0.0f;
  float32 yRes = 0.0f;
  float32 zRes = 0.0f;
  int64 xpNew = 0;
  int64 ypNew = 0;
  int64 zpNew = 0;
  float32 xResNew = 0.0f;
  float32 yResNew = 0.0f;
  float32 zResNew = 0.0f;
  float32 xMinNew = 0.0f;
  float32 yMinNew = 0.0f;
  float32 zMinNew = 0.0f;
};

struct COMPLEXCORE_EXPORT ApplyTransformationToImageGeometryInputValues
{
  DataPath pGeometryToTransform;
  DataPath pCreatedImageGeometry;
  TransformType pTransformationType;
  std::vector<float> pTransformationMatrix;
  int pInterpolationType;
  RotateArgs pRotateArgs;
  bool pUseArraySelector;
  std::vector<DataPath> pSelectedArrays;
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
  // void sendThreadSafeProgressMessage(size_t counter);

private:
  DataStructure& m_DataStructure;
  const ApplyTransformationToImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  // mutable std::mutex m_ProgressMessage_Mutex;
  // size_t m_InstanceIndex = 0;
  // size_t m_TotalElements = 0;
  // size_t m_ProgressCounter = 0;
  // size_t m_LastProgressInt = 0;
};

} // namespace complex

// Result<> ApplyImageTransformation(DataStructure& dataStructure, const ApplyTransformationToImageGeometryInputValues* inputValues, bool shouldCancel);