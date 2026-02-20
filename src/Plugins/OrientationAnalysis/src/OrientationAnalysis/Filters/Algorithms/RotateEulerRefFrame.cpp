#include "RotateEulerRefFrame.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>

#include <EbsdLib/Orientation/AxisAngle.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>

using namespace nx::core;

namespace
{

/**
 * @brief The RotateEulerRefFrameImpl class implements a threaded algorithm that rotates an array of Euler
 * angles about the supplied axis-angle pair.
 */
class RotateEulerRefFrameImpl
{

public:
  RotateEulerRefFrameImpl(Float32Array& data, std::vector<float>& rotAxis, float angle, const std::atomic_bool& shouldCancel, ProgressHelper& progressHelper)
  : m_CellEulerAngles(data)
  , m_AxisAngle(rotAxis)
  , m_Angle(angle)
  , m_ShouldCancel(shouldCancel)
  , m_ProgressHelper(progressHelper)
  {
  }
  virtual ~RotateEulerRefFrameImpl() = default;

  void convert(size_t start, size_t end) const
  {
    ebsdlib::OrientationMatrixDType om = ebsdlib::AxisAngleDType(m_AxisAngle[0], m_AxisAngle[1], m_AxisAngle[2], m_Angle * nx::core::numbers::pi / 180.0).toOrientationMatrix();

    OrientationUtilities::Matrix3dR rotMat = om.toEigenGMatrix();

    ProgressWorker worker = m_ProgressHelper.createWorkerHandle();

    usize counter = 0;
    usize counterIncrement = (end - start) / 100;
    // float ea1 = 0, ea2 = 0, ea3 = 0;
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      if(counter >= counterIncrement)
      {
        worker.incrementProgress(counter);
        counter = 0;
      }

      om = ebsdlib::EulerDType(m_CellEulerAngles[3 * i + 0], m_CellEulerAngles[3 * i + 1], m_CellEulerAngles[3 * i + 2]).toOrientationMatrix();
      OrientationUtilities::Matrix3dR gNew = (om * rotMat).colwise().normalized();

      ebsdlib::EulerDType eu = ebsdlib::OrientationMatrixDType(gNew.data()).toEuler();
      m_CellEulerAngles[3 * i] = eu[0];
      m_CellEulerAngles[3 * i + 1] = eu[1];
      m_CellEulerAngles[3 * i + 2] = eu[2];
      counter++;
    }
    worker.incrementProgress(counter);
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_CellEulerAngles;
  FloatVec3 m_AxisAngle;
  float m_Angle = 0.0F;
  const std::atomic_bool& m_ShouldCancel;
  ProgressHelper& m_ProgressHelper;
};
} // namespace

// -----------------------------------------------------------------------------
RotateEulerRefFrame::RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
RotateEulerRefFrame::~RotateEulerRefFrame() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RotateEulerRefFrame::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  nx::core::Float32Array& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->eulerAngleDataPath);

  size_t totalElements = eulerAngles.getNumberOfTuples();

  nx::core::FloatVec3 axis = {m_InputValues->rotationAxis[0], m_InputValues->rotationAxis[1], m_InputValues->rotationAxis[2]};
  axis = axis.normalize();

  MessageHelper messageHelper(m_MessageHandler);
  ProgressHelper progressHelper =
      messageHelper.createProgressHelper(totalElements, [](usize current, usize max) { return fmt::format("RotateEulerRefFrame: {:.2f}% complete", CalculatePercentComplete(current, max)); });

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalElements);
  dataAlg.execute(RotateEulerRefFrameImpl(eulerAngles, axis, m_InputValues->rotationAxis[3], m_ShouldCancel, progressHelper));
  return {};
}

bool RotateEulerRefFrame::shouldCancel() const
{
  return m_ShouldCancel;
}
