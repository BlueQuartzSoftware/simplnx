#include "RotateEulerRefFrame.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <EbsdLib/Orientation/AxisAngle.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>

#include <cmath>
#include <limits>

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
  RotateEulerRefFrameImpl(RotateEulerRefFrame* filter, Float32Array& data, const FloatVec3& rotAxis, float angle, const std::atomic_bool& shouldCancel)
  : m_CellEulerAngles(data)
  , m_RotationAxis(rotAxis)
  , m_Angle(angle)
  , m_ShouldCancel(shouldCancel)
  , m_Filter(filter)
  {
  }
  ~RotateEulerRefFrameImpl() = default;

  void convert(size_t start, size_t end) const
  {
    // m_Angle arrives in degrees (user-facing parameter) while the Euler angle data is in
    // radians. The axis-angle pair produces the active rotation matrix R, so gNew = g * R
    // implements a passive rotation of the sample reference frame by +angle (right-hand rule).
    ebsdlib::OrientationMatrixDType om = ebsdlib::AxisAngleDType(m_RotationAxis[0], m_RotationAxis[1], m_RotationAxis[2], m_Angle * nx::core::numbers::pi / 180.0).toOrientationMatrix();

    OrientationUtilities::Matrix3dR rotMat = om.toEigenGMatrix();

    usize counter = 0;
    usize counterIncrement = (end - start) / 100;
    for(size_t i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      if(counter >= counterIncrement)
      {
        m_Filter->sendThreadSafeProgressMessage(counter);
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
    m_Filter->sendThreadSafeProgressMessage(counter);
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_CellEulerAngles;
  FloatVec3 m_RotationAxis;
  float m_Angle = 0.0F;
  const std::atomic_bool& m_ShouldCancel;
  RotateEulerRefFrame* m_Filter = nullptr;
};
} // namespace

// -----------------------------------------------------------------------------
RotateEulerRefFrame::RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
RotateEulerRefFrame::~RotateEulerRefFrame() noexcept = default;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void RotateEulerRefFrame::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementPercent(counter);
}

Result<> RotateEulerRefFrame::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  nx::core::Float32Array& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->eulerAngleDataPath);

  size_t totalElements = eulerAngles.getNumberOfTuples();

  nx::core::FloatVec3 axis = {m_InputValues->rotationAxis[0], m_InputValues->rotationAxis[1], m_InputValues->rotationAxis[2]};
  // The filter's preflight rejects a zero-length axis, but guard here as well so that any direct reuse of
  // this Algorithm class cannot silently NaN-corrupt the data (normalize() of a zero vector is NaN).
  const float32 axisMagnitude = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
  if(axisMagnitude < std::numeric_limits<float32>::epsilon())
  {
    return MakeErrorResult(-67050, "The rotation axis has zero length; a rotation axis must be a non-zero vector.");
  }
  axis = axis.normalize();

  m_Throttle.reset(totalElements, "RotateEulerRefFrame");

  // Data-based parallelization: each worker reads and writes only its own disjoint tuple range of the
  // in-place Euler array. Per the project thread-safety policy, concurrent DataStore access is unsafe for
  // out-of-core stores, so requireArraysInMemory disables parallelization unless the array is resident in
  // memory (the codebase-sanctioned pattern; see ConvertOrientations / PartitionGeometry).
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalElements);
  IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&eulerAngles);
  dataAlg.requireArraysInMemory(algArrays);
  dataAlg.execute(RotateEulerRefFrameImpl(this, eulerAngles, axis, m_InputValues->rotationAxis[3], m_ShouldCancel));
  return {};
}

bool RotateEulerRefFrame::shouldCancel() const
{
  return m_ShouldCancel;
}
