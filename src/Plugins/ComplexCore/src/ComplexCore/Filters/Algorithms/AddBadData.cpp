#include "AddBadData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <chrono>
#include <random>

using namespace complex;

namespace
{
struct InitializeTupleToZeroFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& arrayPath, usize index)
  {
    dataStructure.getDataRefAs<DataArray<T>>(arrayPath).initializeTuple(index, 0);
  }
};
} // namespace

// -----------------------------------------------------------------------------
AddBadData::AddBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AddBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AddBadData::~AddBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AddBadData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AddBadData::operator()()
{
  std::mt19937 randomGenerator;
  if(m_InputValues->UseSeed)
  {
    randomGenerator.seed(m_InputValues->SeedValue);
  }
  else
  {
    // time since epoch in milliseconds
    randomGenerator.seed(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
  }

  float32 random = 0.0f;
  auto& imgGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto& GBEuclideanDistances = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->GBEuclideanDistancesArrayPath);

  auto childArrayPaths = GetAllChildArrayDataPaths(m_DataStructure, imgGeom.getCellDataPath());
  auto voxelArrayPaths = childArrayPaths.has_value() ? childArrayPaths.value() : std::vector<DataPath>{};

  size_t totalPoints = GBEuclideanDistances.getSize();
  for(size_t i = 0; i < totalPoints; ++i)
  {
    if(m_InputValues->BoundaryNoise && GBEuclideanDistances[i] < 1)
    {
      random = static_cast<float>(randomGenerator());
      if(random < m_InputValues->BoundaryVolFraction)
      {
        for(const auto& voxelArrayPath : voxelArrayPaths)
        {
          ExecuteDataFunction(InitializeTupleToZeroFunctor{}, m_DataStructure.getDataAs<IDataArray>(voxelArrayPath)->getDataType(), m_DataStructure, voxelArrayPath, i);
        }
      }
    }
    if(m_InputValues->PoissonNoise)
    {
      random = static_cast<float>(randomGenerator());
      if(random < m_InputValues->PoissonVolFraction)
      {
        for(const auto& voxelArrayPath : voxelArrayPaths)
        {
          ExecuteDataFunction(InitializeTupleToZeroFunctor{}, m_DataStructure.getDataAs<IDataArray>(voxelArrayPath)->getDataType(), m_DataStructure, voxelArrayPath, i);
        }
      }
    }
  }

  return {};
}
