Release Notes 1.2.0
===================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.2.0
-------------

API Additions 1.2.0
^^^^^^^^^^^^^^^^^^^

  .. code:: python
  
    cx.Pipeline().to_file(name, file_path)

Filter Changes 1.2.0
^^^^^^^^^^^^^^^^^^^^

- All import and export filters have been renamed to either *ReadXXXX* or *WriteXXXX*. This will effect the python filter classes. 

- SimplnxCore

+----------------------------------------+---------------------------------------+-----------+
| Old File Name                          | New File Name                         | Type      |
+========================================+=======================================+===========+
| WriteAbaqusHexahedronFilter           | WriteAbaqusHexahedronFilter           | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| WriteAvizoRectilinearCoordinateFilter | WriteAvizoRectilinearCoordinateFilter | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| WriteAvizoUniformCoordinateFilter     | WriteAvizoUniformCoordinateFilter     | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| WriteDREAM3DFilter                    | WriteDREAM3DFilter                    | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| WriteFeatureDataCSVFilter             | WriteFeatureDataCSVFilter             | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ReadDeformKeyFileV12Filter           | ReadDeformKeyFileV12Filter            | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ReadVolumeGraphicsFileFilter         | ReadVolumeGraphicsFileFilter          | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ReadBinaryCTNorthstarFilter          | ReadBinaryCTNorthstarFilter           | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| WriteLosAlamosFFTFilter               | WriteLosAlamosFFTFilter               | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ReadRawBinaryFilter                  | ReadRawBinaryFilter                   | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ReadStlFileFilter                    | ReadStlFileFilter                     | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| WriteVtkRectilinearGridFilter         | WriteVtkRectilinearGridFilter         | Filter    |
+----------------------------------------+---------------------------------------+-----------+

- OrientationAnalysis

+------------------------------+-----------------------------+-----------+
| Old File Name                | New File Name               | Type      |
+==============================+=============================+===========+
| ReadEnsembleInfoFilter     | ReadEnsembleInfoFilter      | Filter    |
+------------------------------+-----------------------------+-----------+
| WriteGBCDGMTFileFilter      | WriteGBCDGMTFileFilter      | Filter    |
+------------------------------+-----------------------------+-----------+
| WriteGBCDTriangleDataFilter | WriteGBCDTriangleDataFilter | Filter    |
+------------------------------+-----------------------------+-----------+
| ReadH5Data                 | ReadH5Data                  | Utility   |
+------------------------------+-----------------------------+-----------+
| ReadH5OimDataFilter        | ReadH5OimDataFilter         | Filter    |
+------------------------------+-----------------------------+-----------+
| WriteINLFileFilter              | WriteINLFileFilter          | Filter    |
+------------------------------+-----------------------------+-----------+

