#include "FindBoundaryCells.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
FindBoundaryCells::FindBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundaryCellsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindBoundaryCells::~FindBoundaryCells() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindBoundaryCells::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindBoundaryCells::operator()()
{
  const ImageGeom imageGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const SizeVec3 imageDimensions = imageGeometry.getDimensions();
  const auto xPoints = static_cast<int64>(imageDimensions[0]);
  const auto yPoints = static_cast<int64>(imageDimensions[1]);
  const auto zPoints = static_cast<int64>(imageDimensions[2]);

  auto featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto boundaryCells = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->BoundaryCellsArrayName);

  int64 neighPoints[6] = {0, 0, 0, 0, 0, 0};
  neighPoints[0] = -1 * (xPoints * yPoints);
  neighPoints[1] = -1 * (xPoints);
  neighPoints[2] = -1;
  neighPoints[3] = 1;
  neighPoints[4] = xPoints;
  neighPoints[5] = xPoints * yPoints;

  int32 feature = 0;
  int8 onSurf = 0;
  int32 good = 0;
  int64 neighbor = 0;

  int ignoreFeatureZeroVal = 0;
  if(!m_InputValues->IgnoreFeatureZero)
  {
    ignoreFeatureZeroVal = -1;
  }

  int64 zStride = 0, yStride = 0;
  for(int64 i = 0; i < zPoints; i++)
  {
    zStride = i * xPoints * yPoints;
    for(int64 j = 0; j < yPoints; j++)
    {
      yStride = j * xPoints;
      for(int64 k = 0; k < xPoints; k++)
      {
        onSurf = 0;
        feature = featureIds[zStride + yStride + k];
        if(feature >= 0)
        {
          if(m_InputValues->IncludeVolumeBoundary)
          {
            if(xPoints > 2 && (k == 0 || k == xPoints - 1))
            {
              onSurf++;
            }
            if(yPoints > 2 && (j == 0 || j == yPoints - 1))
            {
              onSurf++;
            }
            if(zPoints > 2 && (i == 0 || i == zPoints - 1))
            {
              onSurf++;
            }

            if(onSurf > 0 && feature == 0)
            {
              onSurf = 0;
            }
          }

          for(int64 l = 0; l < 6; l++)
          {
            good = 1;
            neighbor = zStride + yStride + k + neighPoints[l];
            if(l == 0 && i == 0)
            {
              good = 0;
            }
            if(l == 5 && i == (zPoints - 1))
            {
              good = 0;
            }
            if(l == 1 && j == 0)
            {
              good = 0;
            }
            if(l == 4 && j == (yPoints - 1))
            {
              good = 0;
            }
            if(l == 2 && k == 0)
            {
              good = 0;
            }
            if(l == 3 && k == (xPoints - 1))
            {
              good = 0;
            }
            if(good == 1 && featureIds[neighbor] != feature && featureIds[neighbor] > ignoreFeatureZeroVal)
            {
              onSurf++;
            }
          }
        }
        boundaryCells[zStride + yStride + k] = onSurf;
      }
    }
  }
  return {};
}
