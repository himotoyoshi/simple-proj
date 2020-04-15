require "simple-proj"

#########################################
# -> Web Mercator
#########################################

p "-> Web Mercator"

proj = PROJ.new("+proj=webmerc")

x, y = proj.forward(135, 35)
p [x, y]     ### => [15028131.257091932, 4163881.144064294]

lon, lat = proj.inverse(x, y)
p [lon, lat] ### => [135.0, 35.000000000000014]

x, y = proj.transform_forward(135/180.0*Math::PI, 35/180.0*Math::PI)
                                ### input lon, lat should be in radians for transform_forward

p [x, y]     ### => [15028131.257091932, 4163881.144064294]

#########################################
# +proj=latlong <-> EPSG:3857
#########################################

p "+proj=latlong <-> EPSG:3857"

proj = PROJ.new("EPSG:3857") ### source CRS set to "+proj=latlong" implicitly
p proj.definition

x, y = proj.forward(135, 35)

p [x, y]     ### => [15028131.257091932, 4163881.144064294]

lat, lon = proj.inverse(x, y)

p [lat, lon] ### => [35.00000000000001, 135.0]

x, y = proj.transform_forward(135, 35)

p [x, y]     ### => [15028131.257091932, 4163881.144064294]

#########################################
# EPSG:4326 <-> EPSG:3857
#########################################

p "EPSG:4326 <-> EPSG:3857"

proj = PROJ.new("EPSG:4326", "EPSG:3857")

x, y = proj.transform(35, 135)

p [x, y]     ### => [15028131.257091932, 4163881.144064294]

lat, lon = proj.transform_inverse(x, y)

p [lat, lon] ### => [35.00000000000001, 135.0]

#########################################
# latlong -> EPSG:4326
#########################################

p "latlong <-> EPSG:4326"

proj = PROJ.new("+proj=latlong", "EPSG:4326")

p proj.definition  ### => "+proj=axisswap +order=2,1 +ellps=GRS80"

lon1 = 135.0
lat1 = 35.0

lat2, lon2 = proj.transform_forward(lon1, lat1)
p [lat2, lon2]     ### => [35.0, 135.0]

lon3, lat3 = proj.transform_inverse(lat2, lon2)

p [lon3, lat3]     ### => [135.0, 35.0]
