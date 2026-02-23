#include "ReverseTriangleWinding.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief The ReverseWindingImpl class implements a threaded algorithm that reverses the node
 * windings for a set of triangles
 */
class ReverseWindingImpl
{
public:
  using TriStore = AbstractDataStore<TriangleGeom::SharedFaceList::value_type>;
  explicit ReverseWindingImpl(TriStore& triangles)
  : m_Triangles(triangles)
  {
  }
  ~ReverseWindingImpl() = default;

  void generate(usize start, usize end) const
  {

    for(size_t i = start; i < end; i++)
    {
      // Swap the indices
      TriangleGeom::MeshIndexType nId0 = m_Triangles[i * 3 + 0];
      TriangleGeom::MeshIndexType nId2 = m_Triangles[i * 3 + 2];

      m_Triangles[i * 3 + 0] = nId2;
      m_Triangles[i * 3 + 2] = nId0;
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  TriStore& m_Triangles;
};
} // namespace

// -----------------------------------------------------------------------------
ReverseTriangleWinding::ReverseTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ReverseTriangleWindingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReverseTriangleWinding::~ReverseTriangleWinding() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReverseTriangleWinding::operator()()
{
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->InputTriangleGeometryPath);

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, triangleGeom.getNumberOfFaces());
  dataAlg.execute(ReverseWindingImpl(triangleGeom.getFaces()->getDataStoreRef()));

  return {};
}
