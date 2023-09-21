===================================
Porting ITK Image Processing Filter
===================================


Updating the Legacy UUID Maps
=============================

This **Plugin** contains three folders of filters instructions for porting to and moving to **Filters Folder** are
below:

If Porting From **SIMPL** to **Filters Folder**
-----------------------------------------------

Use the **ComplexFilterGen** module distributed in **SIMPL DREAM3D**. This module will automatically update the
LegacyUUIDMapping for this **Plugin**.

### If Moving to **Filters** ###

.. raw:: html

   <ol>

.. raw:: html

   <li>

Open the LegacyUUIDMapping header file for this Plugin

.. raw:: html

   </li>

.. raw:: html

   <li>

Uncomment the include statement for the filter being moved

.. raw:: html

   </li>

.. raw:: html

   <li>

Uncomment the map entry for the filter being moved

.. raw:: html

   </li>

.. raw:: html

   </ol>

When working with the **LegacyUUIDMapping** header file in this **Plugin** be sure to make sure the commented out tokens
are not removed. Their syntax is one of the following: \*\ **@@\__HEADER\__TOKEN\__DO\__NOT\__DELETE\_\_@@\***

or

\*\ **@@\__MAP\__UPDATE\__TOKEN\__DO\__NOT\__DELETE\_\_@@\***
