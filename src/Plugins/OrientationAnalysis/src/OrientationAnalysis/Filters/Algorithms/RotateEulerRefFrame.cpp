#include "RotateEulerRefFrame.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"

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
  RotateEulerRefFrameImpl(Float32Array& data, std::vector<float>& rotAxis, float angle, const std::atomic_bool& shouldCancel)
  : m_CellEulerAngles(data)
  , m_AxisAngle(rotAxis)
  , m_Angle(angle)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~RotateEulerRefFrameImpl() = default;

  void convert(size_t start, size_t end) const
  {
    float rotMat[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    OrientationTransformation::ax2om<OrientationF, OrientationF>(OrientationF(m_AxisAngle[0], m_AxisAngle[1], m_AxisAngle[2], m_Angle * nx::core::numbers::pi / 180.0F)).toGMatrix(rotMat);

    float ea1 = 0, ea2 = 0, ea3 = 0;
    float g[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float gNew[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    for(size_t i = start; i < end; i++)
    {
      ea1 = m_CellEulerAngles[3 * i + 0];
      ea2 = m_CellEulerAngles[3 * i + 1];
      ea3 = m_CellEulerAngles[3 * i + 2];
      OrientationTransformation::eu2om<OrientationF, OrientationF>(OrientationF(ea1, ea2, ea3)).toGMatrix(g);

      nx::core::MatrixMath::Multiply3x3with3x3(g, rotMat, gNew);
      nx::core::MatrixMath::Normalize3x3(gNew);

      OrientationF eu = OrientationTransformation::om2eu<OrientationF, OrientationF>(OrientationF(gNew));
      m_CellEulerAngles[3 * i] = eu[0];
      m_CellEulerAngles[3 * i + 1] = eu[1];
      m_CellEulerAngles[3 * i + 2] = eu[2];
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_CellEulerAngles;
  std::vector<float> m_AxisAngle;
  float m_Angle = 0.0F;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
RotateEulerRefFrame::RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
RotateEulerRefFrame::~RotateEulerRefFrame() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RotateEulerRefFrame::operator()()
{
  nx::core::Float32Array& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->eulerAngleDataPath);

  size_t m_TotalElements = eulerAngles.getNumberOfTuples();

  std::vector<float> axis = {m_InputValues->rotationAxis[0], m_InputValues->rotationAxis[1], m_InputValues->rotationAxis[2]};
  MatrixMath::Normalize3x1(axis.data());

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, m_TotalElements);
  dataAlg.execute(RotateEulerRefFrameImpl(eulerAngles, axis, m_InputValues->rotationAxis[3], m_ShouldCancel));
  return {};
}
