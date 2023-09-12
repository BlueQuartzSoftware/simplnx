.. _Geometry Descriptions:

Geometry Descriptions
=====================

DREAM3D is capable of working with several different kinds of **Geometries** where a 
goemetry describes where


ImageGeom
--------------

A *structured grid*, this **Geometry** is composed of *pixels* (in 2D) or 
*voxels* (in 3D) with constant spacing along each axis. The voxel is a primary three-dimensional cell. 
The voxel is topologically equivalent to the hexahedron with additional geometric 
constraints. Each face of the voxel is perpendicular to one of the coordinate x-y-z axes.

When creating an ImageGeometry the dimension values are specified as X, Y, Z, *IN THAT ORDER* since this
is most natural way to express a 3-dimensional cube. If you are going to be using an AttributeMatrix with the
Image Geometry, note that the tuple dimensions will be given in **ZYX** order since the tuple dimensions
are **ALWAYS** given in "C" order, or slowest to fastest order.

.. figure:: Images/Image_Geometry.png
   :scale: 50 %
   :alt: Definition of Image Geometry

   Definition of Image Geometry

RectilinearGrid Geometry
-------------------------

An *rectilinear grid*; this **Geometry** is composed of *pixels* (in 2D) or *voxels* 
(in 3D) with variable spacing along each axis. RectilinearGrid represents a geometric structure 
that is topologically regular with variable spacing in the three coordinate directions x-y-z.
To define a RectilinearGrid, you must specify the dimensions of the 3 axis and provide 
three arrays of values specifying the coordinates along the x-y-z axes. The coordinate arrays are 
specified using three DataArray objects (one for X, one for Y, one for Z).

In the figure below, the geometry has dimensions of X=5, Y=3 and Z=1 but note that the number of values
in each array is +1 from the dimension size.


.. figure:: Images/RectGrid_Geometry.png
   :scale: 50 %
   :alt: Definition of RectGrid Geometry

   Definition of RectGrid Geometry


Node Based Geometries
---------------------

These are geometries that consist of an array of vertices (points) where each vertex is
defined by a vector of 3 x 32 bit floating point values representing the X, Y, and Z value for that point. 
The higher dimensional node geometries will also include a second array that defines 
each element (edge, triangle, ...) for that geometry. 

VertexGeometry
^^^^^^^^^^^^^^^^

A collection of points, commonly referred to as a *point cloud*. The vertex is a primary zero-dimensional cell. It is defined by a single point.

  Array of vertex positions
  - float[nV][3]: 12 bytes per vertex
  - (3 coordinates x 4 bytes) per vertex

.. figure:: Images/Vertex_Geometry.png
   :scale: 50 %
   :alt: Definition of Vertex Geometry

   Definition of Vertex Geometry

EdgeGeometry
^^^^^^^^^^^^^^^^

A collection of edges defined by two vertices, forming *lines*. The line is a 
primary one-dimensional cell. It is defined by two points. The direction along the line is from the first point to the second point.

.. figure:: Images/Edge_Geometry.png
   :scale: 50 %
   :alt: Definition of Edge Geometry

   Definition of Edge Geometry

Inherits from VertexGeometry and adds the following:

- array of triples of indices (per triangle)

  - (2 indices x 8 bytes) per triangle
  - 16 bytes per triangle

- represents topology and geometry separately
- finding neighbors is at least well defined

TriangleGeometry
^^^^^^^^^^^^^^^^

The triangle is a primary two-dimensional cell. The triangle is a primary 
two-dimensional cell. The triangle is defined by a counterclockwise 
ordered list of three points. The order of the points specifies the direction of the surface normal using the right-hand rule.

.. figure:: Images/Triangle_Geometry.png
   :scale: 50 %
   :alt: Definition of Triangle Geometry

   Definition of Triangle Geometry

Inherits from VertexGeometry and adds the following:

- array of triples of indices (per triangle)

  - (3 indices x 8 bytes) per triangle
  - 24 bytes per triangle

- represents topology and geometry separately
- finding neighbors is at least well defined



QuadGeometry
^^^^^^^^^^^^^^^^

A collection of quadrilaterals; one type of *surface mesh* . The quadrilateral is a primary 
two-dimensional cell. It is defined by an ordered list of four points lying in a plane. The quadrilateral
is convex and its edges must not intersect. The points are ordered counterclockwise around the quadrilateral, 
defining a surface normal using the right-hand rule.

Inherits from VertexGeometry and adds the following:

- array of triples of indices (per triangle)

  - (4 indices x 8 bytes) per triangle
  - 32 bytes per triangle

- represents topology and geometry separately
- finding neighbors is at least well defined


.. figure:: Images/Quad_Geometry.png
   :scale: 50 %
   :alt: Definition of Quad Geometry

   Definition of Quad Geometry


HexahedralGeometry
^^^^^^^^^^^^^^^^^^^

A collection of hexahedra; one type of *volume mesh*. The hexahedron is a primary three-dimensional 
cell consisting of six quadrilateral faces, twelve edges, and eight vertices. The hexahedron is defined by an ordered list of eight points. The faces and edges must not intersect any other faces and edges, and the hexahedron must be convex.

Inherits from QuadGeometry and adds the following:

- array of triples of indices (per triangle)

  - (8 indices x 8 bytes) per triangle
  - 64 bytes per triangle

- represents topology and geometry separately
- finding neighbors is at least well defined


TetrahedralGeometry
^^^^^^^^^^^^^^^^^^^^

 A collection of tetrahedra; one type of *volume mesh*. The tetrahedron is a primary three-dimensional 
 cell. The tetrahedron is defined by a list of four non-planar points. The tetrahedron has six edges and four triangular faces.
 
 Inherits from TriangleGeometry and adds the following:

- array of triples of indices (per triangle)

  - (4 indices x 8 bytes) per triangle
  - 32 bytes per triangle

- represents topology and geometry separately
- finding neighbors is at least well defined
- 
