#include "VertexGeom.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

VertexGeom::VertexGeom(DataStructure* ds, const std::string& name)
: AbstractGeometry(ds, name)
{
}

VertexGeom::VertexGeom(DataStructure* ds, const std::string& name, size_t numVertices, bool allocate)
: AbstractGeometry(ds, name)
{
}

VertexGeom::VertexGeom(DataStructure* ds, const std::string& name, const SharedVertexList* vertices)
: AbstractGeometry(ds, name)
{
}

VertexGeom::VertexGeom(const VertexGeom& other)
: AbstractGeometry(other)
, m_VertexListId(other.m_VertexListId)
, m_VertexSizesId(other.m_VertexSizesId)
{
}

VertexGeom::VertexGeom(VertexGeom&& other) noexcept
: AbstractGeometry(std::move(other))
, m_VertexListId(std::move(other.m_VertexListId))
, m_VertexSizesId(std::move(other.m_VertexSizesId))
{
}

VertexGeom::~VertexGeom() = default;

DataObject* VertexGeom::shallowCopy()
{
  return new VertexGeom(*this);
}

DataObject* VertexGeom::deepCopy()
{
  throw std::exception();
}

std::string VertexGeom::getGeometryTypeAsString() const
{
  return "VertexGeom";
}

void VertexGeom::initializeWithZeros()
{
}

void VertexGeom::resizeVertexList(size_t newNumVertices)
{
  getVertices()->getDataStore()->resizeTuples(newNumVertices);
}

void VertexGeom::setVertices(const SharedVertexList* vertices)
{
  if(!vertices)
  {
    m_VertexListId.reset();
    return;
  }
  m_VertexListId = vertices->getId();
}

AbstractGeometry::SharedVertexList* VertexGeom::getVertices()
{
  return dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

const AbstractGeometry::SharedVertexList* VertexGeom::getVertices() const
{
  return dynamic_cast<const SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

Point3D<float> VertexGeom::getCoords(size_t vertId) const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return {};
  }
  const size_t offset = vertId * 3;
  Point3D<float> coords;
  for(size_t i = 0; i < 3; i++)
  {
    coords[i] = (*vertices)[offset + i];
  }
}

void VertexGeom::setCoords(size_t vertId, const Point3D<float>& coords)
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }
  const size_t offset = vertId * 3;
  for(size_t i = 0; i < 3; i++)
  {
    (*vertices)[offset + i] = coords[i];
  }
}

size_t VertexGeom::getNumberOfVertices() const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return 0;
  }
  return vertices->getId();
}

size_t VertexGeom::getNumberOfElements() const
{
  return getNumberOfVertices();
}

AbstractGeometry::StatusCode VertexGeom::findElementSizes()
{
  // Vertices are 0-dimensional (they have no size),
  // so simply splat 0 over the sizes array
  auto dataStore = new DataStore<float>(1, getNumberOfElements());
  dataStore->fill(0.0f);

  auto vertexSizes = getDataStructure()->createDataArray<float>("Voxel Sizes", dataStore, getId());
  m_VertexSizesId = vertexSizes->getId();
  return 1;
}

const FloatArray* VertexGeom::getElementSizes() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_VertexSizesId));
}

void VertexGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_VertexSizesId);
  m_VertexSizesId.reset();
}

AbstractGeometry::StatusCode VertexGeom::findElementsContainingVert()
{
  return -1;
}

const AbstractGeometry::ElementDynamicList* VertexGeom::getElementsContainingVert() const
{
  return nullptr;
}

void VertexGeom::deleteElementsContainingVert()
{
}

AbstractGeometry::StatusCode VertexGeom::findElementNeighbors()
{
  return -1;
}

const AbstractGeometry::ElementDynamicList* VertexGeom::getElementNeighbors() const
{
  return nullptr;
}

void VertexGeom::deleteElementNeighbors()
{
}

AbstractGeometry::StatusCode VertexGeom::findElementCentroids()
{
  return -1;
}

const FloatArray* VertexGeom::getElementCentroids() const
{
  return nullptr;
}

void VertexGeom::deleteElementCentroids()
{
}

complex::Point3D<double> VertexGeom::getParametricCenter() const
{
  return { 0.0, 0.0, 0.0 };
}

void VertexGeom::getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const
{
  (void)pCoords;

  shape[0] = 0.0;
  shape[1] = 0.0;
  shape[2] = 0.0;
}

void VertexGeom::findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const
{
  // The exterior derivative of a point source is zero,
  // so simply splat 0 over the derivatives array
  derivatives->getDataStore()->fill(0.0);
}

std::string VertexGeom::getInfoString(complex::InfoStringFormat format) const
{
  if(format == InfoStringFormat::HtmlFormat)
  {
    return getTooltipGenerator().generateHTML();
  }

  return "";
}

complex::TooltipGenerator VertexGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;
  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "VertexGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));

  return toolTipGen;
}

uint32_t VertexGeom::getXdmfGridType() const
{
  throw std::exception();
}

H5::ErrorType VertexGeom::generateXdmfText(std::ostream& out, const std::string& hdfFileName) const
{
  H5::ErrorType err = 0;

  // Always start the grid
  out << "  <!-- *************** START OF " << getName() << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << getName() << R"(" GridType="Uniform">)"
      << "\n";
  if(getTimeSeriesEnabled())
  {
    out << R"(    <Time TimeType="Single" Value=")" << getTimeValue() << "\"/>\n";
  }

  out << R"(    <Topology TopologyType="Polyvertex" NumberOfElements=")" << getNumberOfVertices() << "\">"
      << "\n";
  out << R"(      <DataItem Format="HDF" NumberType="Int" Dimensions=")" << getNumberOfVertices() << "\">"
      << "\n";
  out << "        " << hdfFileName << ":/DataContainers/" << getName() << "/" << "_COMPLEX_GEOMETRY_" << "/"
      << "Verts"
      << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Topology>"
      << "\n";

  out << "    <Geometry Type=\"XYZ\">"
      << "\n";
  out << R"(      <DataItem Format="HDF"  Dimensions=")" << getNumberOfVertices() << R"( 3" NumberType="Float" Precision="4">)"
      << "\n";
  out << "        " << hdfFileName << ":/DataContainers/" << getName() << "/" << "_COMPLEX_GEOMETRY_" << "/" << "SharedVertexList" << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
  out << ""
      << "\n";

  return err;
}

H5::ErrorType VertexGeom::readFromXdmfText(std::istream& in, const std::string& hdfFileName)
{
  throw std::exception();
}

void VertexGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
}

void VertexGeom::setElementNeighbors(const ElementDynamicList* elementNeighbors)
{
}

void VertexGeom::setElementCentroids(const FloatArray* elementCentroids)
{
}

void VertexGeom::setElementSizes(const FloatArray* elementSizes)
{
  if(!elementSizes)
  {
    m_VertexSizesId.reset();
    return;
  }
  m_VertexSizesId = elementSizes->getId();
}
