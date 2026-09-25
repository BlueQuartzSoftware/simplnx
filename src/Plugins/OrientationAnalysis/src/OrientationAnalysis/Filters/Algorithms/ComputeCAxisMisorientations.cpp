#include "ComputeCAxisMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

using namespace nx::core;

namespace
{
constexpr usize k_CAxisComponents = 3;
// Feature arrays are small relative to cell arrays, but we still chunk for OOC correctness.
constexpr usize k_MaxChunkTuples = 65536;
// Minimum acceptable magnitude for the reference direction before normalization.
constexpr float64 k_MinRefDirNorm = 1.0e-10;
} // namespace

// -----------------------------------------------------------------------------
ComputeCAxisMisorientations::ComputeCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ComputeCAxisMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeCAxisMisorientations::~ComputeCAxisMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeCAxisMisorientations::operator()()
{
  // Normalize the reference direction once. Both vectors must be unit length for
  // arccos(dot) to give the correct angle.
  const float64 rx = m_InputValues->ReferenceDir[0];
  const float64 ry = m_InputValues->ReferenceDir[1];
  const float64 rz = m_InputValues->ReferenceDir[2];
  const float64 refNorm = std::sqrt(rx * rx + ry * ry + rz * rz);
  if(refNorm < k_MinRefDirNorm)
  {
    return MakeErrorResult(-77001, "Reference direction has zero magnitude. Provide a non-zero reference direction.");
  }
  const float64 refX = rx / refNorm;
  const float64 refY = ry / refNorm;
  const float64 refZ = rz / refNorm;

  const auto& avgCAxesStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath).getDataStoreRef();
  auto& misorientationsStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MisorientationArrayPath).getDataStoreRef();

  const usize totalFeatures = avgCAxesStoreRef.getNumberOfTuples();
  if(totalFeatures == 0)
  {
    return {};
  }

  const usize chunkTuples = std::min(k_MaxChunkTuples, totalFeatures);
  const auto avgCAxesBuffer = std::make_unique<float32[]>(chunkTuples * k_CAxisComponents);
  const auto misorientationsBuffer = std::make_unique<float32[]>(chunkTuples);

  m_MessageHandler({IFilter::Message::Type::Info, "Computing C-axis misorientations"});

  for(usize tupleOffset = 0; tupleOffset < totalFeatures; tupleOffset += chunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize tupleCount = std::min(chunkTuples, totalFeatures - tupleOffset);

    if(Result<> ioResult = avgCAxesStoreRef.copyIntoBuffer(tupleOffset * k_CAxisComponents, nonstd::span<float32>(avgCAxesBuffer.get(), tupleCount * k_CAxisComponents)); ioResult.invalid())
    {
      return ioResult;
    }

    for(usize tupleIdx = 0; tupleIdx < tupleCount; tupleIdx++)
    {
      const usize cAxisOffset = tupleIdx * k_CAxisComponents;
      const float32 cx = avgCAxesBuffer[cAxisOffset];
      const float32 cy = avgCAxesBuffer[cAxisOffset + 1];
      const float32 cz = avgCAxesBuffer[cAxisOffset + 2];

      // ComputeAvgCAxes marks non-hexagonal features with NaN on all three components.
      if(std::isnan(cx))
      {
        misorientationsBuffer[tupleIdx] = std::numeric_limits<float32>::quiet_NaN();
        continue;
      }

      // AvgCAxes vectors are already normalized. The c-axis has antipodal symmetry
      // ([001] == [00-1] in hexagonal), so take |dot| to keep the result in [0°, 90°].
      const float64 dot = std::clamp(std::abs(cx * refX + cy * refY + cz * refZ), 0.0, 1.0);
      misorientationsBuffer[tupleIdx] = static_cast<float32>(std::acos(dot) * Constants::k_180OverPiD);
    }

    if(Result<> ioResult = misorientationsStoreRef.copyFromBuffer(tupleOffset, nonstd::span<const float32>(misorientationsBuffer.get(), tupleCount)); ioResult.invalid())
    {
      return ioResult;
    }
  }

  m_MessageHandler({IFilter::Message::Type::Info, "C-axis misorientation computation complete"});

  return {};
}
