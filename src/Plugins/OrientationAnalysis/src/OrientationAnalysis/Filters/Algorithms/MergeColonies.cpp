#include "MergeColonies.hpp"

#include "complex/Common/Array.hpp"
#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Utilities/Math/GeometryMath.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace complex;
using LaueOpsShPtrType = std::shared_ptr<LaueOps>;
using LaueOpsContainer = std::vector<LaueOpsShPtrType>;

namespace
{
const float64 unit110 = 1.0 / std::sqrt(2.0);
const float64 unit111 = 1.0 / std::sqrt(3.0);
const float64 unit112_1 = 1.0 / std::sqrt(6.0);
const float64 unit112_2 = 2.0 / std::sqrt(6.0);

float64 crystalDirections[12][3][3] = {{{unit111, unit112_1, unit110}, {-unit111, -unit112_1, unit110}, {unit111, -unit112_2, 0}},

                                       {{-unit111, unit112_1, unit110}, {unit111, -unit112_1, unit110}, {unit111, unit112_2, 0}},

                                       {{unit111, -unit112_1, unit110}, {unit111, -unit112_1, -unit110}, {unit111, unit112_2, 0}},

                                       {{unit111, unit112_1, unit110}, {unit111, unit112_1, -unit110}, {-unit111, unit112_2, 0}},

                                       {{unit111, unit112_1, unit110}, {unit111, -unit112_2, 0}, {unit111, unit112_1, -unit110}},

                                       {{unit111, -unit112_1, unit110}, {-unit111, -unit112_2, 0}, {unit111, -unit112_1, -unit110}},

                                       {{unit111, -unit112_1, unit110}, {unit111, unit112_2, 0}, {-unit111, unit112_1, unit110}},

                                       {{-unit111, -unit112_1, unit110}, {unit111, -unit112_2, 0}, {unit111, unit112_1, unit110}},

                                       {{unit111, -unit112_2, 0}, {unit111, unit112_1, unit110}, {-unit111, -unit112_1, unit110}},

                                       {{unit111, unit112_2, 0}, {-unit111, unit112_1, unit110}, {unit111, -unit112_1, unit110}},

                                       {{unit111, unit112_2, 0}, {unit111, -unit112_1, unit110}, {unit111, -unit112_1, -unit110}},

                                       {{-unit111, unit112_2, 0}, {unit111, unit112_1, unit110}, {unit111, unit112_1, -unit110}}};

class MergeColoniesAlg
{
public:
  MergeColoniesAlg(Int32Array& featureParentIds, AttributeMatrix& cellFeatureAM, Int32Array& featurePhases, float64 axisToleranceRad, float64 angleTolerance)
      :
  , m_OrientationOps(LaueOps::GetAllOrientationOps())
  {
  }
  ~MergeColoniesAlg() = default;

  void execute()
  {
    Int32NeighborList& neighborlist = *(m_ContiguousNeighborList.lock());
    Int32NeighborList& nonContigNeighList = m_NonContiguousNeighborList.lock().get();

    std::vector<int32> grouplist;

    int32 parentcount = 0;
    int32 seed = 0;
    int32 list1size = 0, list2size = 0, listsize = 0;
    int32 neigh = 0;

    while(seed >= 0)
    {
      parentcount++;
      seed = getSeed(parentcount);
      if(seed >= 0)
      {
        grouplist.push_back(seed);
        for(std::vector<int32>::size_type j = 0; j < grouplist.size(); j++)
        {
          int32 firstfeature = grouplist[j];
          list1size = int32(neighborlist[firstfeature].size());
          if(m_UseNonContiguousNeighbors)
          {
            list2size = nonContigNeighList.getListSize(firstfeature);
          }
          for(int32 k = 0; k < 2; k++)
          {
            if(m_PatchGrouping)
            {
              k = 1;
            }
            if(k == 0)
            {
              listsize = list1size;
            }
            else if(k == 1)
            {
              listsize = list2size;
            }
            for(int32 l = 0; l < listsize; l++)
            {
              if(k == 0)
              {
                neigh = neighborlist[firstfeature][l];
              }
              else if(k == 1)
              {
                neigh = nonContigNeighList.getListReference(firstfeature)[l];
              }
              if(neigh != firstfeature)
              {
                if(determineGrouping(firstfeature, neigh, parentcount))
                {
                  if(!m_PatchGrouping)
                  {
                    grouplist.push_back(neigh);
                  }
                }
              }
            }
          }
        }
        if(m_PatchGrouping)
        {
          if(growPatch(parentcount))
          {
            for(std::vector<int32>::size_type j = 0; j < grouplist.size(); j++)
            {
              int32 firstfeature = grouplist[j];
              listsize = int32(neighborlist[firstfeature].size());
              for(int32 l = 0; l < listsize; l++)
              {
                neigh = neighborlist[firstfeature][l];
                if(neigh != firstfeature)
                {
                  if(growGrouping(firstfeature, neigh, parentcount))
                  {
                    grouplist.push_back(neigh);
                  }
                }
              }
            }
          }
        }
      }
      grouplist.clear();
    }
  }

private:
  LaueOpsContainer m_OrientationOps;
  Int32Array& m_FeatureParentIds;
  AttributeMatrix& m_CellFeatureAM;
  Int32Array& m_FeaturePhases;
  Float32Array& m_AvgQuats;
  const UInt32Array& m_CrystalStructures;
  float64 m_AxisToleranceRad = 0.0;
  float64 m_AngleTolerance = 1.0;

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  int32 getSeed(int32 newFid)
  {
    usize numFeatures = m_FeaturePhases.getNumberOfTuples();

    SIMPL_RANDOMNG_NEW();
    int32 seed = -1;
    int32 randFeature = 0;

    // Precalculate some constants
    usize totalFMinus1 = numFeatures - 1;

    usize counter = 0;
    randFeature = int32(float32(rg.genrand_res53()) * float32(totalFMinus1));
    while(seed == -1 && counter < numFeatures)
    {
      if(randFeature > totalFMinus1)
      {
        randFeature = randFeature - numFeatures;
      }
      if(m_FeatureParentIds[randFeature] == -1)
      {
        seed = randFeature;
      }
      randFeature++;
      counter++;
    }
    if(seed >= 0)
    {
      m_FeatureParentIds[seed] = newFid;
      std::vector<usize> tDims(1, newFid + 1);
      m_CellFeatureAM.resizeTuples(tDims);
    }
    return seed;
  }

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  bool check_for_burgers(const QuatD& betaQuat, const QuatD& alphaQuat)
  {
    float64 dP = 0.0;
    float64 angle = 0.0;
    float64 radToDeg = 180.0 / Constants::k_PiD;

    float64 gBeta[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    float64 gBetaT[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    OrientationTransformation::qu2om<QuatD, OrientationD>(betaQuat).toGMatrix(gBeta);
    // transpose gBeta so the sample direction is the output when
    // gBeta is multiplied by the crystal directions below
    MatrixMath::Transpose3x3(gBeta, gBetaT);

    float64 gAlpha[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    float64 gAlphaT[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    OrientationTransformation::qu2om<QuatD, OrientationD>(alphaQuat).toGMatrix(gAlpha);
    // transpose gBeta so the sample direction is the output when
    // gBeta is multiplied by the crystal directions below
    MatrixMath::Transpose3x3(gAlpha, gAlphaT);

    float64 mat[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    for(int32 i = 0; i < 12; i++)
    {
      MatrixMath::Multiply3x3with3x3(gBetaT, crystalDirections[i], mat);
      Point3Dd a = Point3Dd(mat[0][2], mat[1][2], mat[2][2]);
      Point3Dd b = Point3Dd(gAlphaT[0][2], gAlphaT[1][2], gAlphaT[2][2]);
      dP = GeometryMath::CosThetaBetweenVectors(a, b);
      angle = std::acos(dP);
      if((angle * radToDeg) < m_AngleTolerance || (180.0f - (angle * radToDeg)) < m_AngleTolerance)
      {
        a[0] = mat[0][0];
        a[1] = mat[1][0];
        a[2] = mat[2][0];
        b[0] = gAlphaT[0][0];
        b[1] = gAlphaT[1][0];
        b[2] = gAlphaT[2][0];
        dP = GeometryMath::CosThetaBetweenVectors(a, b);
        angle = std::acos(dP);
        if((angle * radToDeg) < m_AngleTolerance)
        {
          return true;
        }
        if((180.0 - (angle * radToDeg)) < m_AngleTolerance)
        {
          return true;
        }
        b[0] = -0.5 * gAlphaT[0][0] + 0.866025 * gAlphaT[0][1];
        b[1] = -0.5 * gAlphaT[1][0] + 0.866025 * gAlphaT[1][1];
        b[2] = -0.5 * gAlphaT[2][0] + 0.866025 * gAlphaT[2][1];
        dP = GeometryMath::CosThetaBetweenVectors(a, b);
        angle = std::acos(dP);
        if((angle * radToDeg) < m_AngleTolerance)
        {
          return true;
        }
        if((180.0 - (angle * radToDeg)) < m_AngleTolerance)
        {
          return true;
        }
        b[0] = -0.5 * gAlphaT[0][0] - 0.866025 * gAlphaT[0][1];
        b[1] = -0.5 * gAlphaT[1][0] - 0.866025 * gAlphaT[1][1];
        b[2] = -0.5 * gAlphaT[2][0] - 0.866025 * gAlphaT[2][1];
        dP = GeometryMath::CosThetaBetweenVectors(a, b);
        angle = std::acos(dP);
        if((angle * radToDeg) < m_AngleTolerance)
        {
          return true;
        }
        if((180.0 - (angle * radToDeg)) < m_AngleTolerance)
        {
          return true;
        }
      }
    }
    return false;
  }

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid)
  {
    float64 w = std::numeric_limits<float64>::max();
    bool colony = false;

    if(m_FeatureParentIds[neighborFeature] == -1 && m_FeaturePhases[referenceFeature] > 0 && m_FeaturePhases[neighborFeature] > 0)
    {
      usize avgQuatIdx = referenceFeature * 4;
      QuatD q1(m_AvgQuats[avgQuatIdx], m_AvgQuats[avgQuatIdx + 1], m_AvgQuats[avgQuatIdx + 2], m_AvgQuats[avgQuatIdx + 3]);
      avgQuatIdx = neighborFeature * 4;
      QuatD q2(m_AvgQuats[avgQuatIdx], m_AvgQuats[avgQuatIdx + 1], m_AvgQuats[avgQuatIdx + 2], m_AvgQuats[avgQuatIdx + 3]);

      uint32 phase1 = m_CrystalStructures[m_FeaturePhases[referenceFeature]];
      uint32 phase2 = m_CrystalStructures[m_FeaturePhases[neighborFeature]];
      if(phase1 == phase2 && (phase1 == EbsdLib::CrystalStructure::Hexagonal_High))
      {
        OrientationD ax = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);

        auto rod = OrientationTransformation::ax2ro<OrientationD, OrientationD>(ax);
        rod = m_OrientationOps[phase1]->getMDFFZRod(rod);
        ax = OrientationTransformation::ro2ax<OrientationD, OrientationD>(rod);

        w = ax[3] * (Constants::k_180OverPiD);
        float angdiff1 = std::fabs(w - 10.53f);
        float axisdiff1 = std::acos(/*std::fabs(n1) * 0.0000f + std::fabs(n2) * 0.0000f +*/ std::fabs(ax[2]) /* * 1.0000f */);
        if(angdiff1 < m_AngleTolerance && axisdiff1 < m_AxisToleranceRad)
        {
          colony = true;
        }
        float angdiff2 = std::fabs(w - 90.00f);
        float axisdiff2 = std::acos(std::fabs(ax[0]) * 0.9958f + std::fabs(ax[1]) * 0.0917f /* + std::fabs(n3) * 0.0000f */);
        if(angdiff2 < m_AngleTolerance && axisdiff2 < m_AxisToleranceRad)
        {
          colony = true;
        }
        float angdiff3 = std::fabs(w - 60.00f);
        float axisdiff3 = std::acos(std::fabs(ax[0]) /* * 1.0000f + std::fabs(n2) * 0.0000f + std::fabs(n3) * 0.0000f*/);
        if(angdiff3 < m_AngleTolerance && axisdiff3 < m_AxisToleranceRad)
        {
          colony = true;
        }
        float angdiff4 = std::fabs(w - 60.83f);
        float axisdiff4 = std::acos(std::fabs(ax[0]) * 0.9834f + std::fabs(ax[1]) * 0.0905f + std::fabs(ax[2]) * 0.1570f);
        if(angdiff4 < m_AngleTolerance && axisdiff4 < m_AxisToleranceRad)
        {
          colony = true;
        }
        float angdiff5 = std::fabs(w - 63.26f);
        float axisdiff5 = std::acos(std::fabs(ax[0]) * 0.9549f /* + std::fabs(n2) * 0.0000f */ + std::fabs(ax[2]) * 0.2969f);
        if(angdiff5 < m_AngleTolerance && axisdiff5 < m_AxisToleranceRad)
        {
          colony = true;
        }
        if(colony)
        {
          m_FeatureParentIds[neighborFeature] = newFid;
          return true;
        }
      }
      else if(EbsdLib::CrystalStructure::Cubic_High == phase2 && EbsdLib::CrystalStructure::Hexagonal_High == phase1)
      {
        colony = check_for_burgers(q2, q1);
        if(colony)
        {
          m_FeatureParentIds[neighborFeature] = newFid;
          return true;
        }
      }
      else if(EbsdLib::CrystalStructure::Cubic_High == phase1 && EbsdLib::CrystalStructure::Hexagonal_High == phase2)
      {
        colony = check_for_burgers(q1, q2);
        if(colony)
        {
          m_FeatureParentIds[neighborFeature] = newFid;
          return true;
        }
      }
    }
    return false;
  }
};
} // namespace

// -----------------------------------------------------------------------------
MergeColonies::MergeColonies(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeColoniesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MergeColonies::~MergeColonies() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& MergeColonies::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> MergeColonies::operator()()
{
  m_AxisToleranceRad = m_AxisTolerance * Constants::k_PiD / 180.0f;

  usize totalFeatures = m_ActivePtr.lock()->getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return MakeErrorResult(-87000, "The number of Grouped Features was 0 or 1 which means no grouped Features were detected. A grouping value may be set too high");
  }

  int32 numParents = 0;
  usize totalPoints = m_FeatureIdsPtr.lock()->getNumberOfTuples();
  for(usize k = 0; k < totalPoints; k++)
  {
    int32 featurename = m_FeatureIds[k];
    m_CellParentIds[k] = m_FeatureParentIds[featurename];
    if(m_FeatureParentIds[featurename] > numParents)
    {
      numParents = m_FeatureParentIds[featurename];
    }
  }
  numParents += 1;

  if(m_RandomizeParentIds)
  {
    // Generate all the numbers up front
    const int32 rangeMin = 1;
    const int32 rangeMax = numParents - 1;
    std::mt19937::result_type seed = static_cast<std::mt19937::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937 generator(seed); // Standard mersenne_twister_engine seeded with milliseconds
    std::uniform_int_distribution<int32> distribution(rangeMin, rangeMax);

    std::vector<int32> pid(numParents);
    pid.push_back(0);
    QSet<int32> parentIdSet;
    parentIdSet.insert(0);
    for(int32 i = 1; i < numParents; ++i)
    {
      pid.push_back(i); // numberGenerator();
      parentIdSet.insert(pid[i]);
    }

    int32 r = 0;
    int32 temp = 0;

    //--- Shuffle elements by randomly exchanging each with one other.
    for(int32 i = 1; i < numParents; i++)
    {
      r = distribution(generator); // Random remaining position.
      if(r >= numParents)
      {
        continue;
      }
      temp = pid[i];
      pid[i] = pid[r];
      pid[r] = temp;
    }

    // Now adjust all the Feature Id values for each Voxel
    for(usize i = 0; i < totalPoints; ++i)
    {
      m_CellParentIds[i] = pid[m_CellParentIds[i]];
      m_FeatureParentIds[m_FeatureIds[i]] = m_CellParentIds[i];
    }
  }

  return {};
}
