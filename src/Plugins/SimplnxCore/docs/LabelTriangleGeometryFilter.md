# Label Triangle Geometry

## Group (Subgroup)

Surface Meshing (Geometry)

## Description

This **Filter** accepts a **Triangle Geometry** and checks the connectivity of **Triangles** and assigns them **region ID**s in a mask accordingly as well as providing a count for the number of **Triangles** in each **Region**.

Note: Our connectivity is different from other connectivity algorithms, in that it is dependent on the dimensionality of the geometry it is run on. A triangle geometry is two dimensional thus it must share one edge with another triangle to be considered connected. Only one shared vertex will not be enough to count triangles as connected. See the following image for reference:

![Connectivity Image](Images/connectivity_image.png)

The image in question demonstrates what geometric shapes would be considered connected according to matching color scheme. The colors are relative to the shapes dimensionality. Thus the fact there is a yellow triangle and a yellow cube does not mean they are related.

- 1D Geometry: Two edges are considered connected if they share a vertex.
- 2D Geometry: Triangle and Quad Geometries are considered connected if they share an entire edge. As shown in the image above, Triangles 1 & 2 are connected while triangles 2 & 3 are **not** connected.
- 3D Geometry: Hexahedron/Tetrahedron: In the image above the hexahedrons 4,5, and 6 are **not** considered connected while 6 & 7 are considered connected.

## Example Output

The image below shows a single STL file that contains 12 different geometries. This filter labeled each geometry with a unique "Region ID" and the visualization is colored by those "Region Ids"

![Filter Output](Images/LabelTriangleGeometry_1.png)

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
