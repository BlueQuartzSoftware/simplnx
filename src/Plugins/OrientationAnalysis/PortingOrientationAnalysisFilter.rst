===================================
Porting Orientation Analysis Filter
===================================


Updating the Legacy UUID Maps
=============================

This **Plugin** contains two folders of filters instructions for both are below:

   .. rubric:: If Porting From **SIMPL** to **Filters Folder**
      :name: if-porting-from-simpl-to-filters-folder

   Use the **ComplexFilterGen** module distributed in **SIMPL DREAM3D**. This module will automatically update the
   LegacyUUIDMapping for this **Plugin**.

   .. rubric:: If Moving to **Filters**
      :name: if-moving-to-filters

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
are not removed. Their syntax is one of the following: > \*\ **@@\__HEADER\__TOKEN\__DO\__NOT\__DELETE\_\_@@\***

or

   \*\ **@@\__MAP\__UPDATE\__TOKEN\__DO\__NOT\__DELETE\_\_@@\***
