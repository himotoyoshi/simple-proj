PROJ4R
=======

Notes for the forward or inverse transformations

    Proj
      forward : (lon, lat) -> (x, y)
      inverse : (x, y) -> (lon, lat)
    
    Proj.transform 
      forward : (src, dst, x, y, z) -> (x', y', z')
      inverse : (dst, src, x, y, z) -> (x', y', z')
    
        If src = latlong, dst = projection, they work like Proj#forward, Proj#inverse.
        In the cases, use lon as x, lat as y.
    
    Geod
      forward : (lat1, lon1, az12, dist) -> (lat2, lon2, az21)
      inverse : (lat1, lon1, lat2, lon2) -> (dist, az12, az21)

Notes for the order of geographic coordinate (lon,lat) or (lat,lon)

    PROJ4::Proj     -> (lon, lat) 
    PROJ4.transform -> (lon, lat) 
    PROJ4::Geod     -> (lat, lon) 

These differences originate from Proj.4 C API.

Proj
----

### Constructor

    pj = PROJ4::Proj.new("+proj=latlon +ellps=WGS84 units=km")

### Attributes

    pj.definition
    pj.latlong?
    pj.geocent?

### Replication

    pj.to_latlong

### forward

    x, y = pj.forward(lon, lat)
    
### inverse

    lon, lat = *pj.inverse(x, y)

p pj.to_latlong.definition

Transformation
--------------


Geod
----

### Constructor

    geod = PROJ4::Geod.new()                           ### [+ellps=WGS84, units=m]
    geod = PROJ4::Geod.new(6378137.0, 1/298.257223563) ### [+ellps=WGS84, units=m]
    geod = PROJ4::Geod.new("+ellps=WGS84 units=m")

### forward

    lat2, lon2, az21 = *geod.forward(lat1, lon1, az12, dist)
    
    input: 
      lat1: latitude of original point in degree
      lon1: longitudde of original point in degree
      az12: azimuth point to target point at origninal point in degree
      dist: distance from original point to target point in units defined in constructor
    
    output: 
      lat2: latitude of target point in degree
      lon1: longitude of target point in degree
      az21: azimuth point to original point at target point in degree

### inverse

    dist, az12, az21 = *geod.inverse(lat1, lon1, lat2, lon2)
     
    input:
      lat1: latitude of original point in degree
      lon1: longitude of original point in degree
      lat2: latitude of target point in degree
      lon2: longitude of target point in degree
     
    output: 
      dist: distance between original and target points in units defined in constructor 
      az12: azimuth point to target point at origninal point in degree
      az21: azimuth point to original point at target point in degree

### distance
 
    dist = *geod.distance(lat1, lon1, lat2, lon2)
    
    input:
      lat1: latitude of original point in degree
      lon1: longitude of original point in degree
      lat2: latitude of target point in degree
      lon2: longitude of target point in degree
    
    output: 
      dist: distance between original and target points in units defined in constructor 
  