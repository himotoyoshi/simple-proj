A Ruby wrapper of PROJ 7 for map projection conversion 
======

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

    PROJ.new(PROJ_STRING)   ### transform defined by proj-string
    PROJ.new(CRS1, CRS2)   ### transform from CRS1 to CRS2
    PROJ.new(CRS2)         ### CRS1 is assumed latlong

The arguments should be String objects one of 

* a proj-string,
* a WKT string,
* an object code (like “EPSG:4326”, “urn:ogc:def:crs:EPSG::4326”, 
  “urn:ogc:def:coordinateOperation:EPSG::1671”),
* a OGC URN combining references for compound coordinate reference 
  systems (e.g “urn:ogc:def:crs,crs:EPSG::2393,crs:EPSG::5717” or 
  custom abbreviated syntax “EPSG:2393+5717”),
* a OGC URN combining references for concatenated operations (e.g. 
  “urn:ogc:def:coordinateOperation,coordinateOperation:EPSG::3895,
  coordinateOperation:EPSG::1618”)

### Foward transformation

    PROJ#forward(lon1, lat1, z1=nil)        =>  x2, y2[, z2]
    PROJ#transform_forward(x1, y1, z1=nil)  =>  x2, y2[, z2]

The units of input parameters lon1 and lat1 are degrees.

### Inverse transformation

    PROJ#inverse(x1, y1, z1=nil)            =>  lon2, lat2[, z2]
    PROJ#transform_inverse(x1, y1, z1=nil)  =>  x2, y2[, z2]

The units of output parameters lon2 and lat2 are degrees.

Examples
--------

### Conversion between 'latlong' and 'Web Mercator'

```ruby
#########################################
# latlong <-> Web Mercator
#########################################
proj = PROJ.new("+proj=webmerc")

x, y = proj.forward(135, 35)
p [x, y]     ### => [15028131.257091932, 4163881.144064294]

lon, lat = proj.inverse(x, y)
p [lon, lat] ### => [135.0, 35.000000000000014]
```

### Conversion between 'EPSG:4326' and 'EPSG:3857'

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

### Definition of conversion between 'latlong' to 'EPSG:4326'

```ruby
#########################################
# latlong -> EPSG:4326
#########################################

proj = PROJ.new("+proj=latlong", "EPSG:4326")

p proj.definition  ### => "+proj=axisswap +order=2,1 +ellps=GRS80"

lon1 = 135.0
lat1 = 35.0

lat2, lon2 = proj.transform_forward(lon1, lat1)
p [lat2, lon2]     ### => [15028131.257091932, 4163881.144064294]

lon3, lat3 = proj.transform_inverse(lat2, lon2)

p [lon3, lat3]     ### => [35.00000000000001, 135.0]
```

