Simple PROJ
===========

This is a ruby extension library for map projection conversion using PROJ.
Note that this library is not a verbatim wrapper for all functions in PROJ.

Installation
------------

    gem install simple-proj

### Requirement

* PROJ version 6 or 7

Features
--------

* Conversion of latitude and longitude to map projection coordinates (forward transformation)
* Conversion of map projection coordinates to latitude and longitude (inverse transformation)
* Transformation a map projection coordinate to another map projection coordinate


Usage
-----

### Constructor

    PROJ.new(PROJ_STRING)  ### transform defined by proj-string
    PROJ.new(CRS1, CRS2)   ### transform from CRS1 to CRS2
    PROJ.new(CRS2)         ### CRS1 is assumed to be "+proj=latlong +type=crs"

The arguments should be String objects one of 

* a proj-string,
* a WKT string,
* an object code (like 'EPSG:4326', 'urn:ogc:def:crs:EPSG::4326', 
  'urn:ogc:def:coordinateOperation:EPSG::1671'),
* a OGC URN combining references for compound coordinate reference 
  systems (e.g 'urn:ogc:def:crs,crs:EPSG::2393,crs:EPSG::5717' or 
  custom abbreviated syntax 'EPSG:2393+5717'),
* a OGC URN combining references for concatenated operations (e.g. 
  'urn:ogc:def:coordinateOperation,coordinateOperation:EPSG::3895,
  coordinateOperation:EPSG::1618')

### Transformation

Forward transformation.

    PROJ#transform(x1, y1, z1=nil)  =>  x2, y2[, z2]
    PROJ#transform_forward(x1, y1, z1=nil)  =>  x2, y2[, z2]   ### alias of #transform

Inverse transformation.

    PROJ#transform_inverse(x1, y1, z1=nil)  =>  x2, y2[, z2]


### Special methods for transformation from geodetic coordinates and other coordinates.

These are special methods provided to avoid converting 
between latitude and longitude units between degrees and radians.

    PROJ#forward(lon1, lat1, z1=nil)        =>  x2, y2[, z2]
    PROJ#inverse(x1, y1, z1=nil)            =>  lon2, lat2[, z2]

The units of input parameters for PROJ#forward are degrees.
Internally, these units are converted to radians and passed to proj_trans().
Similarly, the units of output parameters for PROJ#inverse are degrees.

```ruby
proj = PROJ.new("+proj=webmerc")

x, y = proj.forward(135, 35)
p [x, y]     ### => [15028131.257091932, 4163881.144064294]

lon, lat = proj.inverse(x, y)
p [lon, lat] ### => [135.0, 35.000000000000014]

```

Note: 
This method should not be used for the PROJ object initialized with the proj-string that do not expect geodetic coordinates in radians for the source CRS. In such case, use  #transform_forward and #transform_inverse instead of #forward and #inverse. 
This is an example for *invalid* usage. 

```ruby
pj = PROJ.new("+proj=unitconvert +xy_in=deg +xy_out=rad")

p pj.forward(135, 35)    ### does not work as expected
# => [0.041123351671205656, 0.0106616096925348]

p pj.transform(135, 35)  ### works as expected
# => [2.356194490192345, 0.6108652381980153]
```

Examples
--------

### Conversion between 'EPSG:4326' and 'EPSG:3857'

For EPSG:4326 (in PROJ), the geographic coordinates are in the order of latitude and longitude.
Then, the transformation from EPSG:4326 requires the arguments in the order of latitude and longitude.

```ruby
#########################################
# EPSG:4326 <-> EPSG:3857
#########################################

proj = PROJ.new("EPSG:4326", "EPSG:3857")

x, y = proj.transform(35, 135)

p [x, y]     ### => [15028131.257091932, 4163881.144064294]

lat, lon = proj.transform_inverse(x, y)

p [lat, lon] ### => [35.00000000000001, 135.0]
```



