#include "ErodeDilateMask.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

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

  std::array<int64, 6> neighpoints = {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};

  for(int32_t iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    for(size_t j = 0; j < totalPoints; j++)
    {
      maskCopy[j] = mask[j];
    }
    for(int64 zIndex = 0; zIndex < dims[2]; zIndex++)
    {
      const int64 zStride = dims[0] * dims[1] * zIndex;
      for(int64 yIndex = 0; yIndex < dims[1]; yIndex++)
      {
        const int64 yStride = dims[0] * yIndex;
        for(int64 xIndex = 0; xIndex < dims[0]; xIndex++)
        {
          const int64 voxelIndex = zStride + yStride + xIndex;

          if(!mask[voxelIndex])
          {
            for(int32_t neighPointIdx = 0; neighPointIdx < 6; neighPointIdx++)
            {
              const int64 neighpoint = voxelIndex + neighpoints[neighPointIdx];
              if(neighPointIdx == 0 && (zIndex == 0 || !m_InputValues->ZDirOn))
              {
                continue;
              }
              if(neighPointIdx == 5 && (zIndex == (dims[2] - 1) || !m_InputValues->ZDirOn))
              {
                continue;
              }
              if(neighPointIdx == 1 && (yIndex == 0 || !m_InputValues->YDirOn))
              {
                continue;
              }
              if(neighPointIdx == 4 && (yIndex == (dims[1] - 1) || !m_InputValues->YDirOn))
              {
                continue;
              }
              if(neighPointIdx == 2 && (xIndex == 0 || !m_InputValues->XDirOn))
              {
                continue;
              }
              if(neighPointIdx == 3 && (xIndex == (dims[0] - 1) || !m_InputValues->XDirOn))
              {
                continue;
              }

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
    }
    for(size_t j = 0; j < totalPoints; j++)
    {
      mask[j] = maskCopy[j];
    }
  }

  return {};
}
