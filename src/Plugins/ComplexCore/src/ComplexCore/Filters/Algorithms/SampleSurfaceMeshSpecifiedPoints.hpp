#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Utilities/SampleSurfaceMesh.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT SampleSurfaceMeshSpecifiedPointsInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  FileSystemPathParameter::ValueType InputFilePath;
  FileSystemPathParameter::ValueType OutputFilePath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */
class COMPLEXCORE_EXPORT SampleSurfaceMeshSpecifiedPoints : public SampleSurfaceMesh
{
public:
  SampleSurfaceMeshSpecifiedPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   SampleSurfaceMeshSpecifiedPointsInputValues* inputValues);
  ~SampleSurfaceMeshSpecifiedPoints() noexcept;

  SampleSurfaceMeshSpecifiedPoints(const SampleSurfaceMeshSpecifiedPoints&) = delete;
  SampleSurfaceMeshSpecifiedPoints(SampleSurfaceMeshSpecifiedPoints&&) noexcept = delete;
  SampleSurfaceMeshSpecifiedPoints& operator=(const SampleSurfaceMeshSpecifiedPoints&) = delete;
  SampleSurfaceMeshSpecifiedPoints& operator=(SampleSurfaceMeshSpecifiedPoints&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SampleSurfaceMeshSpecifiedPointsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
