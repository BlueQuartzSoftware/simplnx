#include "ErodeDilateMask.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ErodeDilateMask::ErodeDilateMask(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateMaskInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateMask::~ErodeDilateMask() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateMask::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateMask::operator()()
{

  auto& mask = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->MaskArrayPath);
  const size_t totalPoints = mask.getNumberOfTuples();

  std::vector<bool> maskCopy(totalPoints, false);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(dims[2]);
  progressHelper.setProgressMessageTemplate("ErodeDilateMask: {:.1f}% Complete");

  for(int32_t iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {}", iteration));

    progressHelper.resetProgress();
    auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

    for(size_t j = 0; j < totalPoints; j++)
    {
      maskCopy[j] = mask[j];
    }
    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      const int64 zStride = dims[0] * dims[1] * zIdx;
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        const int64 yStride = dims[0] * yIdx;
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = zStride + yStride + xIdx;

          if(!mask[voxelIndex])
          {
            // Loop over the 6 face neighbors of the voxel
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              const int64 neighpoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

              if(m_InputValues->Operation == detail::k_DilateIndex && mask[neighpoint])
              {
                maskCopy[voxelIndex] = true;
              }
              if(m_InputValues->Operation == detail::k_ErodeIndex && mask[neighpoint])
              {
                maskCopy[neighpoint] = false;
              }
            }
          }
        }
      }
      progressMessenger.sendProgressMessage(1);
    }
    for(size_t j = 0; j < totalPoints; j++)
    {
      mask[j] = maskCopy[j];
    }
  }

  return {};
}
