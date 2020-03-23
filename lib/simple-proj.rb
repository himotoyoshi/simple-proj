require 'simple_proj.so'

class PROJ
    
  def definition
    text = _definition
    if text
      return text.strip.split(/\s+/).map{|s| "+"+s}.join(" ")
    else
      nil
    end
    
  end
    
  alias transform_forward transform
  
  alias forward_lonlat forward
  
  def forward_latlon (lat, lon, z = nil)
    return forward(lon, lat, z)
  end

  alias inverse_lonlat inverse
  
  def inverse_latlon (x, y, z = nil)
    return inverse_latlon(x, y, z)
  end
    
end