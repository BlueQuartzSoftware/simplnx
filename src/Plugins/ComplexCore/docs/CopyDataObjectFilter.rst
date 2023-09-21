================
Copy Data Object
================


Group (Subgroup)
================

Core (Generation)

Description
===========

This **Filter** deep copies one or more DataObjects.

\**In the case of copying \_DataObject_s that inherit from *BaseGroup*\ **, such as DataGroup or AttributeMatrix,** it
will copy all of the child objects recursively*\*, that is to say all of an object’s children and childrens’ children
and so on will be copied if applicable.

Commonly used *BaseGroup* children: - **ALL** Geometries - *DataGroup* - *AttributeMatrix* - *GridMontage*

See the DataStructure section of the reference manual for a complete hierarchy.

When the *Copy to New Parent* is toggled true a new parameter will appear. This parameter, *Copied Parent Group*, allows
for the selected arrays to all be copied into whatever data container you place here.

Parameters
==========

None

Required Geometry
=================

Not Applicable

Required Objects
================

+-----------+---------------------------+-------------------------+-----------------+--------------------------------+
| Kind      | Default Name              | Type                    | Comp Dims       | Description                    |
+===========+===========================+=========================+=================+================================+
| D         | N/A                       | N/A                     | N/A             | The list of DataObjects to     |
| ataObject |                           |                         |                 | copy.                          |
+-----------+---------------------------+-------------------------+-----------------+--------------------------------+
| bool      | false                     | N/A                     | N/A             | Whether to copy the            |
|           |                           |                         |                 | DataObjects to a new parent or |
|           |                           |                         |                 | not.                           |
+-----------+---------------------------+-------------------------+-----------------+--------------------------------+
| DataGroup | N/A                       | N/A                     | N/A             | The group to be used as the    |
|           |                           |                         |                 | parent for all the DataObject  |
|           |                           |                         |                 | copies if the Copy to New      |
|           |                           |                         |                 | Parent option is selected      |
+-----------+---------------------------+-------------------------+-----------------+--------------------------------+
| string    | \_COPY                    | N/A                     | N/A             | The suffix string to be        |
|           |                           |                         |                 | appended to each copy’s name   |
+-----------+---------------------------+-------------------------+-----------------+--------------------------------+

Created Objects
===============

A deep copy of the DataObjects selected in the input.

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
