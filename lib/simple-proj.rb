require 'simple_proj_ext'
require 'json'
require 'bindata'
require 'ostruct'

class PROJ

  VERSION = _info["version"]

  # Returns PROJ info
  #
  # @return [OpenStruct]
  def self.info
    return OpenStruct.new(_info)
  end

  alias transform_forward transform
  
  alias forward_lonlat forward
  
  # A variant of #forward which accept the axis order as (lat, lon).
  def forward_latlon (lat, lon, z = nil)
    return forward(lon, lat, z)
  end

  alias inverse_lonlat inverse
  
  # A variant of #inverse which return the output with the axis order in (lat, lon).
  def inverse_latlon (x, y, z = nil)
    return inverse_latlon(x, y, z)
  end

  # Returns a internal information of the object
  #
  # @return [OpenStruct]
  def pj_info
    info = _pj_info
    if info["id"] == "unknown"
      transform(0,0)
      info = _pj_info
      if info["id"] == "unknown"
        return OpenStruct.new(info)
      else
        return pj_info
      end
    else
      info["definition"] = info["definition"].strip.split(/\s+/).map{|s| "+"+s}.join(" ")
      return OpenStruct.new(info)
    end
  end

  # Returns a definition of the object
  #
  # @return [OpenStruct]
  def definition
    return pj_info.definition
  end

  ENDIAN = ( [1].pack("I") == [1].pack("N") ) ? :big : :little
  
  # A class represents PJ_FACTORS structure defined using BinData::Record
  # 
  # This structure has members, 
  # 
  # * meridional_scale
  # * parallel_scale
  # * areal_scale
  # * angular_distortion
  # * meridian_parallel_angle
  # * meridian_convergence
  # * tissot_semimajor
  # * tissot_semiminor
  # * dx_dlam
  # * dx_dphi
  # * dy_dlam
  # * dy_dphi
  #
  class FACTORS < BinData::Record
    endian ENDIAN
    double :meridional_scale
    double :parallel_scale
    double :areal_scale
    double :angular_distortion
    double :meridian_parallel_angle
    double :meridian_convergence
    double :tissot_semimajor
    double :tissot_semiminor
    double :dx_dlam
    double :dx_dphi
    double :dy_dlam
    double :dy_dphi
  end

  # Returns PROJ::FACTORS object
  #
  # @retrun [PROJ::FACTORS]
  def factors (lon, lat)
    return FACTORS.read(_factors(lon, lat))
  end

end

class PROJ
  
  module Common

    # Gets a EPSG code of the object
    #
    # @return [String, nil]
    def to_epsg_code
      auth = id_auth_name
      code = id_code
      if auth and code
        return auth + ":" + code
      else
        return nil
      end
    end

    # Gets a Hash object parsed from the PROJJSON expression of the object 
    #
    # @return [Hash, nil]
    def to_projjson_as_hash
      json = to_projjson
      if json
        return JSON.parse(json)
      else
        return nil
      end      
    end

    def to_wkt2_2015
      return to_wkt(WKT2_2015)
    end

    def to_wkt2_2015_simplified
      return to_wkt(WKT2_2015_SIMPLIFIED)
    end

    def to_wkt2_2018
      return to_wkt(WKT2_2018)
    end
    
    def to_wkt2_2018_simplified
      if defined? WKT2_2018_SIMPLIFIED
        return to_wkt(WKT2_2018_SIMPLIFIED)
      else
        raise "WKT2_2018 not defined. Check PROJ version."
      end
    end

    def to_wkt_gdal
      return to_wkt(WKT1_GDAL)
    end

    def to_wkt_esri
      return to_wkt(WKT1_ESRI)
    end
 
  end

end

### Marshalling

class PROJ
  
  def _dump_data 
    return to_wkt
  end
  
  def _load_data (wkt)
    initialize_copy self.class.new(wkt)
  end
  
end

class PROJ::CRS
  
  def _dump_data 
    return to_wkt
  end
  
  def _load_data (wkt)
    initialize_copy self.class.new(wkt)
  end
  
end

begin
  require "simple-proj-carray"
rescue LoadError
end
