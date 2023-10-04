# Porting ITK Image Processing Filter

## Updating the Legacy UUID Maps

This **Plugin** contains three folders of filters instructions for porting to
and moving to ***Filters Folder*** are below:

### If Porting From ***SIMPL*** to ***Filters Folder***

 Use the **ComplexFilterGen** module distributed in ***SIMPL DREAM3D***. This
module will automatically update the LegacyUUIDMapping for this **Plugin**.

### If Moving to ***Filters***

 <ol>
  <li> Open the LegacyUUIDMapping header file for this Plugin </li>
  <li> Uncomment the include statement for the filter being moved </li>
  <li> Uncomment the map entry for the filter being moved </li>
</ol>
  
 When working with the ***LegacyUUIDMapping*** header file in this **Plugin**
 be sure to make sure the commented out tokens are not removed. Their syntax is
 one of the following:
  ***@@**HEADER__TOKEN__DO__NOT__DELETE**@@***

 or

  ***@@**MAP__UPDATE__TOKEN__DO__NOT__DELETE**@@***
