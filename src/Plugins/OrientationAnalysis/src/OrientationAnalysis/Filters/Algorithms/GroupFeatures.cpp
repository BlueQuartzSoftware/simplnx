#include "GroupFeatures.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/NeighborList.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
GroupFeatures::GroupFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GroupFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_GroupInputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GroupFeatures::~GroupFeatures() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GroupFeatures::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
int GroupFeatures::getSeed(int32 newFid)
{
  return -1;
}

// -----------------------------------------------------------------------------
bool GroupFeatures::determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid)
{
  return false;
}

// -----------------------------------------------------------------------------
bool GroupFeatures::growPatch(int32_t currentPatch)
{
  return false;
}

// -----------------------------------------------------------------------------
bool GroupFeatures::growGrouping(int32_t referenceFeature, int32_t neighborFeature, int32_t newFid)
{
  return false;
}

// -----------------------------------------------------------------------------
Result<> GroupFeatures::operator()()
{
  execute();
  return {};
}

void GroupFeatures::execute()
{
  NeighborList<int32>& neighborlist = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_GroupInputValues->ContiguousNeighborListArrayPath);
  NeighborList<int32>* nonContigNeighList = nullptr;
  if(m_GroupInputValues->UseNonContiguousNeighbors)
  {
    nonContigNeighList = m_DataStructure.getDataAs<NeighborList<int32>>(m_GroupInputValues->NonContiguousNeighborListArrayPath);
  }

  std::vector<int32> grouplist;

  int32 parentcount = 0;
  int32 seed = 0;
  int32 list1size = 0, list2size = 0, listsize = 0;
  int32 neigh = 0;
  bool patchGrouping = false;

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
        if(m_GroupInputValues->UseNonContiguousNeighbors)
        {
          list2size = nonContigNeighList->getListSize(firstfeature);
        }
        for(int32 k = 0; k < 2; k++)
        {
          if(patchGrouping)
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
              neigh = nonContigNeighList->getListReference(firstfeature)[l];
            }
            if(neigh != firstfeature)
            {
              if(determineGrouping(firstfeature, neigh, parentcount))
              {
                if(!patchGrouping)
                {
                  grouplist.push_back(neigh);
                }
              }
            }
          }
        }
      }
      if(patchGrouping)
      {
        if(growPatch(parentcount))
        {
          for(std::vector<int32_t>::size_type j = 0; j < grouplist.size(); j++)
          {
            int32_t firstfeature = grouplist[j];
            listsize = int32_t(neighborlist[firstfeature].size());
            for(int32_t l = 0; l < listsize; l++)
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
