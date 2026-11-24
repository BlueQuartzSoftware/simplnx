Unreleased API Changes
======================

Geometry metadata validation
----------------------------

In C++, both overloads of ``ImageGeom::setOrigin()`` and ``ImageGeom::setSpacing()`` return
``[[nodiscard]] Result<>``. The origin must be finite on each axis. The spacing must be finite
and greater than zero on each axis. A failed call leaves the geometry unchanged.
A newly created ``ImageGeom`` starts with spacing ``[1, 1, 1]`` instead of ``[0, 0, 0]``.
``ImageGeom::findElementSizes()`` no longer returns error -1530 for nonpositive spacing.

``RectGridGeom::setBounds()`` also marks its ``Result<>`` as ``[[nodiscard]]``. Each supplied
bounds array must have one component per tuple and at least two finite values in strictly
increasing order. A failed call leaves all three bounds associations unchanged.

Check the result before using the updated metadata. A function that returns ``Result<>``
can propagate the error directly.

.. code-block:: cpp

   if(auto result = imageGeom.setSpacing(0.5F, 0.5F, 1.0F); result.invalid())
   {
     return result;
   }

In Python, the geometry metadata properties are read-only. Check the ``simplnx.Result``
from geometry creation helpers or actions before using the geometry.
See :ref:`Creating an Image Geometry <creating-an-image-geometry>`.


Geometry metadata in file readers
---------------------------------

The following readers have linked **Set Origin** and **Set Spacing** options. Enable either
option to supply that field in X, Y, Z order.

* Read EDAX EBSD Data (.ang)
* Read Oxford Instr. EBSD Data (.ctf)
* Read Oxford Instr. Channel 5 (.cpr/.crc)
* Read H5EBSD File
* Read Vtk Structured Points File
* Read MHA/MetaImage File
* Read NIfTI File (Version 1)
* Read Zeiss TXM/TXRM Files
* Read North Star Imaging CT (.nsihdr/.nsidat)
* Read Volume Graphics File (.vgi/.vol)

Readers now reject explicit invalid spacing that some formats previously treated as 1.
This affects MHA spacing, CTF ``ZStep``, NRRD spacing, TIFF resolution, and NIfTI spacing.
Enable **Set Spacing** and supply finite, positive values to read a file with invalid spacing.
An omitted MHA spacing field or CTF ``ZStep`` still uses 1. In NRRD ``space directions``, a leading
``none`` marks the component axis. Once axis 0 is the component axis, a later spatial ``none``
uses 1 as the spacing on that axis. When ``space directions`` is absent, a spatial ``nan``, ``+nan``,
or ``-nan`` in ``spacings`` uses 1.
NIfTI uses the magnitude of finite, negative ``pixdim`` values. A zero, NaN, or infinite
spacing component still requires an override. An explicit zero or negative TIFF resolution
requires **Set Spacing**.

**Read Image** now honors **Set Origin** and **Set Spacing** when reading ``.nrrd`` and
``.nhdr`` files. Existing NRRD pipelines that enable either option now use the configured
metadata for the resulting image. **Preprocessed** applies the overrides before cropping,
so a physical crop can select different voxels. **Postprocessed** applies the overrides
after cropping and uses valid stored metadata to determine the crop. If an enabled
override replaces an invalid stored value, the crop uses the replacement for that field.

**Read DREAM3D-NX File** validates the geometry metadata of objects selected for import. Enable
**Use Default Origin/Spacing for Invalid Values** to replace each invalid origin component
with 0 and each invalid spacing component with 1. The filter reports warning 502693 for each
affected image geometry. Rectilinear bounds always require finite, strictly increasing values.

Geometry action validation in preflight
---------------------------------------

Preflight applies ``CreateImageGeometryAction``, ``UpdateImageGeomAction``, and
``CreateRectGridGeometryAction`` with geometry validation. Invalid computed image origin or
spacing now fails preflight with an error where it previously produced a warning (#1952).
Rectilinear grid creation requires at least two bounds per axis (error -4006). Actions propagate
setter failures to the caller. DREAM.3D writes reject invalid rectilinear bounds.

Remove the fifth constructor argument from existing ``UpdateImageGeomAction`` calls.
The action now validates the origin and spacing in both modes. Its previous codes -86028
through -86030 have been removed.

During execution, ``CreateRectGridGeometryAction`` in Create mode initializes each bounds
array in increasing order starting with 0, 1, 2, and so on.
