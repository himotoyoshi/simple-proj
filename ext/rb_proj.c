#include "ruby.h"
#include "rb_proj.h"

VALUE rb_cProj;
VALUE rb_cCrs;
VALUE rb_mCommon;

ID id_forward;
ID id_inverse;

static VALUE
rb_proj_info (VALUE klass)
{
  volatile VALUE vout;
  PJ_INFO info;

  info = proj_info();

  vout = rb_hash_new();
  rb_hash_aset(vout, rb_str_new2("major"), INT2NUM(info.major));
  rb_hash_aset(vout, rb_str_new2("minor"), INT2NUM(info.minor));
  rb_hash_aset(vout, rb_str_new2("patch"), INT2NUM(info.patch));
  rb_hash_aset(vout, rb_str_new2("release"), rb_str_new2(info.release));
  rb_hash_aset(vout, rb_str_new2("version"), rb_str_new2(info.version));
  rb_hash_aset(vout, rb_str_new2("searchpath"), rb_str_new2(info.searchpath));

  return vout;
}

static void 
free_proj (Proj *proj)
{
  if ( proj->ref ) {
    proj_destroy(proj->ref);
  }
  free(proj);
}

static VALUE
rb_proj_s_allocate (VALUE klass)
{
  Proj *proj;
  return Data_Make_Struct(klass, Proj, 0, free_proj, proj);
}

/*
Constructs a transformation object with one or two arguments.
The arguments should be PROJ::CRS objects or String objects one of 

 * a proj-string,
 * a WKT string,
 * an object code (like “EPSG:4326”, “urn:ogc:def:crs:EPSG::4326”, 
   “urn:ogc:def:coordinateOperation:EPSG::1671”),
 * an Object name. e.g “WGS 84”, “WGS 84 / UTM zone 31N”. 
   In that case as uniqueness is not guaranteed, 
   heuristics are applied to determine the appropriate best match.
 * a OGC URN combining references for compound coordinate reference 
   systems (e.g “urn:ogc:def:crs,crs:EPSG::2393,crs:EPSG::5717” or 
   custom abbreviated syntax “EPSG:2393+5717”),
 * a OGC URN combining references for concatenated operations (e.g. 
   “urn:ogc:def:coordinateOperation,coordinateOperation:EPSG::3895,
   coordinateOperation:EPSG::1618”)
 * a PROJJSON string. 
   The jsonschema is at https://proj.org/schemas/v0.4/projjson.schema.json (added in PROJ 6.2)
 * a compound CRS made from two object names separated with ” + “. 
   e.g. “WGS 84 + EGM96 height” (added in 7.1)

If two arguments are given, the first is the source CRS definition and the
second is the target CRS definition. If only one argument is given, the
following two cases are possible. 

 * a proj-string, which represents a transformation. 
 * a CRS defintion, in which case the latlong coordinates are implicitly 
   used as the source CRS definition.
 * a PROJ::CRS object

@overload initialize(def1, def2=nil)
  @param def1 [String] proj-string or other CRS definition (see above description).
  @param def2 [String, nil] proj-string or other CRS definition (see above description).

@example
  # Transformation from EPSG:4326 to EPSG:3857
  pj = PROJ.new("EPSG:4326", "EPSG:3857")

  # Transformation from (lat,lon) to WEB Mercator.
  pj = PROJ.new("+proj=webmerc")

  # Transformation from (lat,lon) to EPSG:3857
  pj = PROJ.new("EPSG:3857")

  # Using PROJ::CRS objects
  epsg_3857 = PROJ::CRS.new("EPSG:3857")
  pj = PROJ.new(epsg_3857)
  pj = PROJ.new("EPSG:4326", epsg_3857)
  pj = PROJ.new(epsg_3857, "EPSG:4326")

*/

static VALUE
rb_proj_initialize (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vdef1, vdef2;
  Proj *proj, *crs;
  PJ *ref, *src;
  PJ_TYPE type;
  int errno;

  rb_scan_args(argc, argv, "11", (VALUE *)&vdef1, (VALUE *)&vdef2);

  Data_Get_Struct(self, Proj, proj);

  if ( NIL_P(vdef2) ) {
    if ( rb_obj_is_kind_of(vdef1, rb_cCrs) ) {
      Data_Get_Struct(vdef1, Proj, crs);
      vdef1 = rb_str_new2(proj_as_proj_string(PJ_DEFAULT_CTX, crs->ref, PJ_PROJ_5, NULL));
    }
    else {
      Check_Type(vdef1, T_STRING);
    }
    ref = proj_create(PJ_DEFAULT_CTX, StringValuePtr(vdef1));      
    if ( proj_is_crs(ref) ) {
      proj_destroy(ref);
      ref = proj_create_crs_to_crs(PJ_DEFAULT_CTX, "+proj=latlong +type=crs", StringValuePtr(vdef1), NULL);    
      proj->ref = ref;
      proj->is_src_latlong = 2;      
    }
    else {
      proj->ref = ref;
      proj->is_src_latlong = 1;      
    }
  }
  else {
    if ( rb_obj_is_kind_of(vdef1, rb_cCrs) ) {
      Data_Get_Struct(vdef1, Proj, crs);
      vdef1 = rb_str_new2(proj_as_proj_string(PJ_DEFAULT_CTX, crs->ref, PJ_PROJ_5, NULL));
    }
    else {
      Check_Type(vdef1, T_STRING);
    }
    if ( rb_obj_is_kind_of(vdef2, rb_cCrs) ) {
      Data_Get_Struct(vdef2, Proj, crs);
      vdef2 = rb_str_new2(proj_as_proj_string(PJ_DEFAULT_CTX, crs->ref, PJ_PROJ_5, NULL));
    }
    else {
      Check_Type(vdef2, T_STRING);
    }
    ref = proj_create_crs_to_crs(PJ_DEFAULT_CTX, StringValuePtr(vdef1), StringValuePtr(vdef2), NULL);    
    proj->ref = ref;
    src = proj_get_source_crs(PJ_DEFAULT_CTX, ref);
    type = proj_get_type(src);
    if ( type == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
         type == PJ_TYPE_GEOGRAPHIC_3D_CRS ) {
      proj->is_src_latlong = 2;      
    }
    else {
      proj->is_src_latlong = 0;      
    }
  }
  
  if ( ! ref ) {
    errno = proj_context_errno(PJ_DEFAULT_CTX);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }
  
  return Qnil;
}

/*
Normalizes the axis order which is the one expected for visualization purposes.
If the axis order of its source or target CRS is
northing, easting, then an axis swap operation will be inserted.

@return [self]
*/
static VALUE
rb_proj_normalize_for_visualization (VALUE self)
{
  Proj *proj;
  PJ *ref, *orig;
  int errno;

  Data_Get_Struct(self, Proj, proj);
  orig = proj->ref;

  ref = proj_normalize_for_visualization(PJ_DEFAULT_CTX, orig);
  if ( ! ref ) {
    errno = proj_context_errno(PJ_DEFAULT_CTX);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }

  proj->ref = ref;

  proj_destroy(orig);
    
  return self;  
}

/*
Returns source CRS as PROJ::CRS object.

@return [PROJ, nil]
*/
static VALUE
rb_proj_source_crs (VALUE self)
{
  Proj *proj;
  PJ *crs;

  Data_Get_Struct(self, Proj, proj);
  crs = proj_get_source_crs(PJ_DEFAULT_CTX, proj->ref);

  if ( ! crs ) {
    return Qnil;
  }

  return rb_crs_new(crs);
}

/*
Returns target CRS as PROJ::CRS object.

@return [PROJ, nil]
*/
static VALUE
rb_proj_target_crs (VALUE self)
{
  Proj *proj;
  PJ *crs;

  Data_Get_Struct(self, Proj, proj);
  crs = proj_get_target_crs(PJ_DEFAULT_CTX, proj->ref);

  if ( ! crs ) {
    return Qnil;
  }
  
  return rb_crs_new(crs);
}

/*
Checks if a operation expects input in radians or not.

@return [Boolean]
*/

static VALUE
rb_proj_angular_input (VALUE self, VALUE direction)
{
  Proj *proj;

  Data_Get_Struct(self, Proj, proj);
  
  if ( rb_to_id(direction) == id_forward ) {
    return proj_angular_input(proj->ref, PJ_FWD) == 1 ? Qtrue : Qfalse;
  }
  else if ( rb_to_id(direction) == id_inverse ) {
    return proj_angular_input(proj->ref, PJ_INV) == 1 ? Qtrue : Qfalse;    
  }
  else {
    rb_raise(rb_eArgError, "invalid direction");
  }
}

/*
Checks if an operation returns output in radians or not.

@return [Boolean] 
*/

static VALUE
rb_proj_angular_output (VALUE self, VALUE direction)
{
  Proj *proj;

  Data_Get_Struct(self, Proj, proj);
  
  if ( rb_to_id(direction) == id_forward ) {
    return proj_angular_output(proj->ref, PJ_FWD) == 1 ? Qtrue : Qfalse;
  }
  else if ( rb_to_id(direction) == id_inverse ) {
    return proj_angular_output(proj->ref, PJ_INV) == 1 ? Qtrue : Qfalse;    
  }
  else {
    rb_raise(rb_eArgError, "invalid direction");
  }
}

static VALUE
rb_proj_pj_info (VALUE self)
{
  volatile VALUE vout;
  Proj *proj;
  PJ_PROJ_INFO info;

  Data_Get_Struct(self, Proj, proj);
  info = proj_pj_info(proj->ref);

  vout = rb_hash_new();
  rb_hash_aset(vout, rb_str_new2("id"), (info.id) ? rb_str_new2(info.id) : Qnil);
  rb_hash_aset(vout, rb_str_new2("description"), rb_str_new2(info.description));
  rb_hash_aset(vout, rb_str_new2("definition"), rb_str_new2(info.definition));
  rb_hash_aset(vout, rb_str_new2("has_inverse"), INT2NUM(info.has_inverse));
  rb_hash_aset(vout, rb_str_new2("accuracy"), rb_float_new(info.accuracy));

  return vout;
}


static VALUE
rb_proj_factors (VALUE self, VALUE vlon, VALUE vlat)
{
  Proj *proj;
  PJ_COORD pos;
  PJ_FACTORS factors;
  
  Data_Get_Struct(self, Proj, proj);

  pos.lp.lam = proj_torad(NUM2DBL(vlon));
  pos.lp.phi = proj_torad(NUM2DBL(vlat));
  
  factors = proj_factors(proj->ref, pos);

  return rb_str_new((const char *) &factors, sizeof(PJ_FACTORS));
}


/*
Transforms coordinates forwardly from (lat1, lon1, z1) to (x1, y2, z2).
The order of coordinates arguments should be longitude, latitude, and height.
The input longitude and latitude should be in units 'degrees'.
If the returned coordinates are angles, they are converted in units `degrees`.

@overload forward(lon1, lat1, z1 = nil)
  @param lon1 [Numeric] longitude in degrees.
  @param lat1 [Numeric] latitude in degrees.
  @param z1 [Numeric, nil] vertical coordinate.

@return x2, y2[, z2]

@example
  x2, y2 = pj.forward(lon1, lat1)
  x2, y2, z2 = pj.forward(lon1, lat1, z1)

*/
static VALUE
rb_proj_forward (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vlon, vlat, vz;
  Proj *proj;
  PJ_COORD data_in, data_out;
  int errno;

  rb_scan_args(argc, argv, "21", (VALUE*) &vlon, (VALUE*) &vlat, (VALUE*) &vz);

  Data_Get_Struct(self, Proj, proj);

  if ( ! proj->is_src_latlong ) {
    rb_raise(rb_eRuntimeError, "requires latlong src crs. use #transform_forward instead of #forward.");
  }

  if ( proj_angular_input(proj->ref, PJ_FWD) == 1 ) {
    data_in.lpz.lam = proj_torad(NUM2DBL(vlon));
    data_in.lpz.phi = proj_torad(NUM2DBL(vlat));
    data_in.lpz.z   = NIL_P(vz) ? 0.0 : NUM2DBL(vz);
  }
  else {
    data_in.xyz.x = NUM2DBL(vlon);
    data_in.xyz.y = NUM2DBL(vlat);
    data_in.xyz.z = NIL_P(vz) ? 0.0 : NUM2DBL(vz);    
  }

  data_out = proj_trans(proj->ref, PJ_FWD, data_in);

  if ( data_out.xyz.x == HUGE_VAL ) {
    errno = proj_context_errno(PJ_DEFAULT_CTX);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }

  if (  proj_angular_output(proj->ref, PJ_FWD) == 1 ) {    
    if ( NIL_P(vz) ) {
      return rb_assoc_new(rb_float_new(proj_todeg(data_out.lpz.lam)), 
                          rb_float_new(proj_todeg(data_out.lpz.phi)));
    } else {
      return rb_ary_new3(3, rb_float_new(proj_todeg(data_out.lpz.lam)), 
                            rb_float_new(proj_todeg(data_out.lpz.phi)),
                            rb_float_new(data_out.lpz.z));
    }
  }
  else {
    if ( NIL_P(vz) ) {
      return rb_assoc_new(rb_float_new(data_out.xyz.x), 
                          rb_float_new(data_out.xyz.y));    
    } else {
      return rb_ary_new3(3, rb_float_new(data_out.xyz.x), 
                            rb_float_new(data_out.xyz.y),
                            rb_float_new(data_out.xyz.z));        
    }
  }
  
}

/*
Transforms coordinates forwardly from (lat1, lon1, z1) to (x1, y2, z2).
The order of coordinates arguments should be longitude, latitude, and height.
The input longitude and latitude should be in units 'degrees'.
If the returned coordinates are angles, they are treated as in units `radians`.

@overload forward(lon1, lat1, z1 = nil)
  @param lon1 [Numeric] longitude in degrees.
  @param lat1 [Numeric] latitude in degrees.
  @param z1 [Numeric, nil] vertical coordinate.

@return x2, y2[, z2]

@example
  x2, y2 = pj.forward(lon1, lat1)
  x2, y2, z2 = pj.forward(lon1, lat1, z1)

*/
static VALUE
rb_proj_forward_bang (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vlon, vlat, vz;
  Proj *proj;
  PJ_COORD data_in, data_out;
  int errno;

  rb_scan_args(argc, argv, "21", (VALUE*) &vlon, (VALUE*) &vlat, (VALUE*) &vz);

  Data_Get_Struct(self, Proj, proj);

  if ( ! proj->is_src_latlong ) {
    rb_raise(rb_eRuntimeError, "requires latlong src crs. use #transform_forward instead of #forward.");
  }

  if ( proj_angular_input(proj->ref, PJ_FWD) == 1 ) {
    data_in.lpz.lam = proj_torad(NUM2DBL(vlon));
    data_in.lpz.phi = proj_torad(NUM2DBL(vlat));
    data_in.lpz.z   = NIL_P(vz) ? 0.0 : NUM2DBL(vz);
  }
  else {
    data_in.xyz.x = NUM2DBL(vlon);
    data_in.xyz.y = NUM2DBL(vlat);
    data_in.xyz.z = NIL_P(vz) ? 0.0 : NUM2DBL(vz);    
  }

  data_out = proj_trans(proj->ref, PJ_FWD, data_in);

  if ( data_out.xyz.x == HUGE_VAL ) {
    errno = proj_context_errno(PJ_DEFAULT_CTX);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }

  if ( NIL_P(vz) ) {
    return rb_assoc_new(rb_float_new(data_out.xyz.x), 
                        rb_float_new(data_out.xyz.y));    
  } else {
    return rb_ary_new3(3, rb_float_new(data_out.xyz.x), 
                          rb_float_new(data_out.xyz.y),
                          rb_float_new(data_out.xyz.z));        
  }
  
}

/*
Transforms coordinates inversely from (x1, y1, z1) to (lon2, lat2, z2).
The order of output coordinates is longitude, latitude and height.
If the input coordinates are angles, they are treated as being in units `degrees`.
The returned longitude and latitude are in units 'degrees'.

@overload inverse(x1, y1, z1 = nil)
  @param x1 [Numeric] 
  @param y1 [Numeric]
  @param z1 [Numeric, nil]

@return lon2, lat2, [, z2]

@example
  lon2, lat2 = pj.inverse(x1, y1)
  lon2, lat2, z2 = pj.inverse(x1, y1, z1)

*/
static VALUE
rb_proj_inverse (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vx, vy, vz;
  Proj *proj;
  PJ_COORD data_in, data_out;
  int errno;

  rb_scan_args(argc, argv, "21", (VALUE *)&vx, (VALUE *)&vy, (VALUE *)&vz);
  Data_Get_Struct(self, Proj, proj);

  if ( ! proj->is_src_latlong ) {
    rb_raise(rb_eRuntimeError, "requires latlong src crs. use #transform_inverse instead of #inverse.");
  }

  if ( proj_angular_input(proj->ref, PJ_INV) == 1 ) {
    data_in.lpz.lam = proj_torad(NUM2DBL(vx));
    data_in.lpz.phi = proj_torad(NUM2DBL(vy));
    data_in.lpz.z   = NIL_P(vz) ? 0.0 : NUM2DBL(vz);
  }
  else {
    data_in.xyz.x = NUM2DBL(vx);
    data_in.xyz.y = NUM2DBL(vy);
    data_in.xyz.z = NIL_P(vz) ? 0.0 : NUM2DBL(vz);
  }

  data_out = proj_trans(proj->ref, PJ_INV, data_in);

  if ( data_out.lpz.lam == HUGE_VAL ) {
    errno = proj_errno(proj->ref);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }

  if (  proj_angular_output(proj->ref, PJ_INV) == 1 ) {    
    if ( NIL_P(vz) ) {
      return rb_assoc_new(rb_float_new(proj_todeg(data_out.lpz.lam)), 
                          rb_float_new(proj_todeg(data_out.lpz.phi)));
    } else {
      return rb_ary_new3(3, rb_float_new(proj_todeg(data_out.lpz.lam)), 
                            rb_float_new(proj_todeg(data_out.lpz.phi)),
                            rb_float_new(data_out.lpz.z));
    }
  }
  else {
    if ( NIL_P(vz) ) {
      return rb_assoc_new(rb_float_new(data_out.xyz.x), 
                          rb_float_new(data_out.xyz.y));
    } else {
      return rb_ary_new3(3, rb_float_new(data_out.xyz.x), 
                            rb_float_new(data_out.xyz.y),
                            rb_float_new(data_out.xyz.z));
    }    
  }
}

/*
Transforms coordinates inversely from (x1, y1, z1) to (lon2, lat2, z2).
The order of output coordinates is longitude, latitude and height.
If the input coordinates are angles, they are treated as being in units `radians`.
The returned longitude and latitude are in units 'degrees'.

@overload inverse(x1, y1, z1 = nil)
  @param x1 [Numeric] 
  @param y1 [Numeric]
  @param z1 [Numeric, nil]

@return lon2, lat2, [, z2]

@example
  lon2, lat2 = pj.inverse(x1, y1)
  lon2, lat2, z2 = pj.inverse(x1, y1, z1)

*/
static VALUE
rb_proj_inverse_bang (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vx, vy, vz;
  Proj *proj;
  PJ_COORD data_in, data_out;
  int errno;

  rb_scan_args(argc, argv, "21", (VALUE *)&vx, (VALUE *)&vy, (VALUE *)&vz);
  Data_Get_Struct(self, Proj, proj);

  if ( ! proj->is_src_latlong ) {
    rb_raise(rb_eRuntimeError, "requires latlong src crs. use #transform_inverse instead of #inverse.");
  }

  data_in.xyz.x = NUM2DBL(vx);
  data_in.xyz.y = NUM2DBL(vy);
  data_in.xyz.z = NIL_P(vz) ? 0.0 : NUM2DBL(vz);

  data_out = proj_trans(proj->ref, PJ_INV, data_in);

  if ( data_out.lpz.lam == HUGE_VAL ) {
    errno = proj_errno(proj->ref);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }

  if (  proj_angular_output(proj->ref, PJ_INV) == 1 ) {    
    if ( NIL_P(vz) ) {
      return rb_assoc_new(rb_float_new(proj_todeg(data_out.lpz.lam)), 
                          rb_float_new(proj_todeg(data_out.lpz.phi)));
    } else {
      return rb_ary_new3(3, rb_float_new(proj_todeg(data_out.lpz.lam)), 
                            rb_float_new(proj_todeg(data_out.lpz.phi)),
                            rb_float_new(data_out.lpz.z));
    }
  }
  else {
    if ( NIL_P(vz) ) {
      return rb_assoc_new(rb_float_new(data_out.xyz.x), 
                          rb_float_new(data_out.xyz.y));
    } else {
      return rb_ary_new3(3, rb_float_new(data_out.xyz.x), 
                            rb_float_new(data_out.xyz.y),
                            rb_float_new(data_out.xyz.z));
    }    
  }
}

static VALUE
rb_proj_transform_i (int argc, VALUE *argv, VALUE self, PJ_DIRECTION direction)
{
  VALUE vx, vy, vz;
  Proj *trans;
  PJ_COORD c_in, c_out;
  int errno;

  rb_scan_args(argc, argv, "21", (VALUE*)&vx, (VALUE*)&vy, (VALUE*)&vz);

  Data_Get_Struct(self, Proj, trans);

  c_in.xyz.x = NUM2DBL(vx);
  c_in.xyz.y = NUM2DBL(vy);
  c_in.xyz.z = NIL_P(vz) ? 0.0 : NUM2DBL(vz);      

  c_out = proj_trans(trans->ref, direction, c_in);

  if ( c_out.xyz.x == HUGE_VAL ) {
    errno = proj_errno(trans->ref);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }
  
  if ( NIL_P(vz) ) {
    return rb_ary_new3(2, 
    	     rb_float_new(c_out.xyz.x),
	         rb_float_new(c_out.xyz.y));      
  }
  else {
    return rb_ary_new3(3, 
    	     rb_float_new(c_out.xyz.x),
	         rb_float_new(c_out.xyz.y),
	         rb_float_new(c_out.xyz.z));
  }      
  
}

/*
Transforms coordinates forwardly from (x1, y1, z1) to (x1, y2, z2).
The order of coordinates arguments are according to source and target CRSs.

@overload transform_forward(x1, y1, z1 = nil)
  @param x1 [Numeric]
  @param y1 [Numeric]
  @param z1 [Numeric, nil]

@return x2, y2[, z2] (Numeric)

@example
  x2, y2 = pj.transform(x1, y1)
  x2, y2, z2 = pj.transform(x1, y1, z1)

*/
static VALUE
rb_proj_transform_forward (int argc, VALUE *argv, VALUE self)
{
  return rb_proj_transform_i(argc, argv, self, PJ_FWD);
}

/*
Transforms coordinates inversely from (x1, y1, z1) to (x1, y2, z2).
The order of coordinates arguments are according to source and target CRSs.

@overload transform_inverse(x1, y1, z1 = nil)
  @param x1 [Numeric]
  @param y1 [Numeric]
  @param z1 [Numeric]
@return x2, y2[, z2] (Numeric)

@example
  x2, y2 = pj.transform_inverse(x1, y1)
  x2, y2, z2 = pj.transform_inverse(x1, y1, z1)

*/
static VALUE
rb_proj_transform_inverse (int argc, VALUE *argv, VALUE self)
{
  return rb_proj_transform_i(argc, argv, self, PJ_INV);
}

/*
Constructs a CRS object with an argument.
The argument should be a String object one of 

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

@overload initialize(definition)
  @param definition [String] proj-string or other CRS definition (see above description).

@example
  # Create a PROJ::CRS object
  epsg_3857 = PROJ::CRS.new("EPSG:3857")

*/

static VALUE
rb_crs_initialize (VALUE self, VALUE vdef1)
{
  Proj *proj;
  PJ *ref;
  int errno;

  Data_Get_Struct(self, Proj, proj);

  ref = proj_create(PJ_DEFAULT_CTX, StringValuePtr(vdef1));

  if ( ! ref ) {
    errno = proj_context_errno(PJ_DEFAULT_CTX);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }

  if ( proj_is_crs(ref) ) {
    proj->ref = ref;
  }
  else {
    rb_raise(rb_eRuntimeError, "should be crs definition");
  }
    
  return Qnil;
}

VALUE 
rb_crs_new (PJ *ref) 
{
  volatile VALUE vcrs;
  Proj *proj;
  
  vcrs = rb_proj_s_allocate(rb_cCrs);
  Data_Get_Struct(vcrs, Proj, proj);
  
  proj->ref = ref;
  
  return vcrs;
}


static VALUE
rb_proj_initialize_copy (VALUE self, VALUE obj)
{
  Proj *proj, *other;

  Data_Get_Struct(self, Proj, proj);

  if ( rb_obj_is_kind_of(obj, rb_cProj) || rb_obj_is_kind_of(obj, rb_cCrs) ) {
    Data_Get_Struct(obj, Proj, other);
    proj->ref = proj_clone(PJ_DEFAULT_CTX, other->ref);    
  }
  else {
    rb_raise(rb_eArgError, "invalid class of argument object");
  }

  return self;
}

/*
Gets the name of the object.

@return [String]
*/
static VALUE
rb_proj_get_name (VALUE self)
{
  Proj *proj;

  Data_Get_Struct(self, Proj, proj);

  return rb_str_new2(proj_get_name(proj->ref));  
}

/*
Gets the authority name / codespace of an identifier of the object.

@overload id_auth_name (index=nil)
  @param index [Integer] index of the identifier (0 = first identifier)

@return [String, nil]
*/

static VALUE
rb_proj_get_id_auth_name (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vidx;
  Proj *proj;
  const char *string;

  rb_scan_args(argc, argv, "01", (VALUE *)&vidx);

  Data_Get_Struct(self, Proj, proj);

  if ( NIL_P(vidx) ) {
    string = proj_get_id_auth_name(proj->ref, 0);
  }
  else {
    string = proj_get_id_auth_name(proj->ref, NUM2INT(vidx));
  }

  return (string) ? rb_str_new2(string) : Qnil;
}

/*
Gets the code of an identifier of the object.

@overload id_code (index=nil)
  @param index [Integer] index of the identifier (0 = first identifier)

@return [String, nil]
*/

static VALUE
rb_proj_get_id_code (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vidx;
  Proj *proj;
  const char *string;

  rb_scan_args(argc, argv, "01", (VALUE *)&vidx);

  Data_Get_Struct(self, Proj, proj);

  if ( NIL_P(vidx) ) {
    string = proj_get_id_code(proj->ref, 0);    
  }
  else {
    string = proj_get_id_code(proj->ref, NUM2INT(vidx));        
  }

  return (string) ? rb_str_new2(string) : Qnil;
}


/*
Gets a PROJ string representation of the object.
This method may return nil if the object is not compatible with 
an export to the requested type.

@return [String,nil]
*/
static VALUE
rb_proj_as_proj_string (VALUE self)
{
  Proj *proj;
  const char *string;

  Data_Get_Struct(self, Proj, proj);
  string = proj_as_proj_string(PJ_DEFAULT_CTX, proj->ref, PJ_PROJ_5, NULL);
  if ( ! string ) {
    return Qnil;
  }
  return rb_str_new2(string);
}

#if PROJ_AT_LEAST_VERSION(6,2,0)

/*
Gets a PROJJSON string representation of the object.

This method may return nil if the object is not compatible 
with an export to the requested type.

@return [String,nil]
*/

static VALUE
rb_proj_as_projjson (int argc, VALUE *argv, VALUE self)
{
  Proj *proj;
	volatile VALUE vopts;
  const char *options[4] = {NULL, NULL, NULL, NULL};
  const char *json = NULL;
	int i;

  Data_Get_Struct(self, Proj, proj);

	if ( argc == 0 ) {
    json = proj_as_projjson(PJ_DEFAULT_CTX, proj->ref, NULL);		
	}
	if ( argc > 3 ) {
		rb_raise(rb_eRuntimeError, "too much options");
	}
	else {
		for (i=0; i<argc; i++) {
	    Check_Type(argv[i], T_STRING);			
			options[i] = StringValuePtr(argv[i]);
		}
    json = proj_as_projjson(PJ_DEFAULT_CTX, proj->ref, options);				
	}

  if ( ! json ) {
    return Qnil;
  }
  return rb_str_new2(json);
}

#endif


/*
Gets a ellipsoid parameters of CRS definition of the object.

@return [Array] Returns Array containing semi_major_axis(m), semi_minor(m), boolean whether the semi-minor value was computed, inverse flattening.
*/

static VALUE
rb_proj_ellipsoid_get_parameters (VALUE self)
{
  Proj *proj;
  PJ *ellps;
  double a, b, invf;
  int computed;

  Data_Get_Struct(self, Proj, proj);
  ellps = proj_get_ellipsoid(PJ_DEFAULT_CTX, proj->ref);
  proj_ellipsoid_get_parameters(PJ_DEFAULT_CTX, ellps, &a, &b, &computed, &invf);
  return rb_ary_new3(4,
                     rb_float_new(a),
                     rb_float_new(b),
                     INT2NUM(computed),
                     rb_float_new(invf));
}

/*
Gets a WKT expression of CRS definition of the object.

@return [String] Returns String of WKT expression.
*/
static VALUE
rb_proj_as_wkt (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vidx;
  Proj *proj;
  const char *wkt;

  rb_scan_args(argc, argv, "01", (VALUE *)&vidx);

  Data_Get_Struct(self, Proj, proj);

  if ( NIL_P(vidx) ) {
    wkt = proj_as_wkt(PJ_DEFAULT_CTX, proj->ref, PJ_WKT2_2018, NULL);    
  }
  else {
    wkt = proj_as_wkt(PJ_DEFAULT_CTX, proj->ref, NUM2INT(vidx), NULL);        
  }

  if ( ! wkt ) {
    return Qnil;
  }
  else {
    return rb_str_new2(wkt);    
  }
}


void
Init_simple_proj_ext ()
{
  id_forward = rb_intern("forward");
  id_inverse = rb_intern("inverse");

  rb_cProj = rb_define_class("PROJ", rb_cObject);
  rb_cCrs = rb_define_class_under(rb_cProj, "CRS", rb_cObject);
  rb_mCommon = rb_define_module_under(rb_cProj, "Common");

  rb_include_module(rb_cProj, rb_mCommon);
  rb_include_module(rb_cCrs,  rb_mCommon);

  rb_define_singleton_method(rb_cProj, "_info", rb_proj_info, 0);

  rb_define_alloc_func(rb_cProj, rb_proj_s_allocate);
  rb_define_method(rb_cProj, "initialize", rb_proj_initialize, -1);
  rb_define_method(rb_cProj, "source_crs", rb_proj_source_crs, 0);
  rb_define_method(rb_cProj, "target_crs", rb_proj_target_crs, 0);
  rb_define_method(rb_cProj, "angular_input?", rb_proj_angular_input, 1);
  rb_define_method(rb_cProj, "angular_output?", rb_proj_angular_output, 1);
  rb_define_method(rb_cProj, "normalize_for_visualization", rb_proj_normalize_for_visualization, 0);
  rb_define_method(rb_cProj, "forward", rb_proj_forward, -1);
  rb_define_method(rb_cProj, "forward!", rb_proj_forward_bang, -1);
  rb_define_method(rb_cProj, "inverse", rb_proj_inverse, -1);
  rb_define_method(rb_cProj, "inverse!", rb_proj_inverse_bang, -1);
  rb_define_method(rb_cProj, "transform", rb_proj_transform_forward, -1);
  rb_define_method(rb_cProj, "transform_inverse", rb_proj_transform_inverse, -1);
  rb_define_private_method(rb_cProj, "_pj_info", rb_proj_pj_info, 0);
  rb_define_private_method(rb_cProj, "_factors", rb_proj_factors, 2);
  
  rb_define_alloc_func(rb_cCrs, rb_proj_s_allocate);
  rb_define_method(rb_cCrs, "initialize", rb_crs_initialize, 1);
  rb_define_method(rb_cCrs, "normalize_for_visualization", rb_proj_normalize_for_visualization, 0);

  rb_define_method(rb_mCommon, "initialize_copy", rb_proj_initialize_copy, 1);
  rb_define_method(rb_mCommon, "name", rb_proj_get_name, 0);
  rb_define_method(rb_mCommon, "id_auth_name", rb_proj_get_id_auth_name, -1);
  rb_define_method(rb_mCommon, "id_code", rb_proj_get_id_code, -1);
  rb_define_method(rb_mCommon, "to_proj_string", rb_proj_as_proj_string, 0);
#if PROJ_AT_LEAST_VERSION(6,2,0)
  rb_define_method(rb_mCommon, "to_projjson", rb_proj_as_projjson, -1);
#endif
  rb_define_method(rb_mCommon, "ellipsoid_parameters", rb_proj_ellipsoid_get_parameters, 0);
  rb_define_method(rb_mCommon, "to_wkt", rb_proj_as_wkt, -1);

  rb_define_const(rb_cProj, "WKT2_2015", INT2NUM(PJ_WKT2_2015));
  rb_define_const(rb_cProj, "WKT2_2015_SIMPLIFIED", INT2NUM(PJ_WKT2_2015_SIMPLIFIED));
#ifdef WKT2_2018
  rb_define_const(rb_cProj, "WKT2_2018", INT2NUM(PJ_WKT2_2018));
  rb_define_const(rb_cProj, "WKT2_2018_SIMPLIFIED", INT2NUM(PJ_WKT2_2018_SIMPLIFIED));
#endif
#ifdef WKT2_2019
  rb_define_const(rb_cProj, "WKT2_2019", INT2NUM(PJ_WKT2_2019));
  rb_define_const(rb_cProj, "WKT2_2019_SIMPLIFIED", INT2NUM(PJ_WKT2_2019_SIMPLIFIED));
#endif
  rb_define_const(rb_cProj, "WKT1_GDAL", INT2NUM(PJ_WKT1_GDAL));
  rb_define_const(rb_cProj, "WKT1_ESRI", INT2NUM(PJ_WKT1_ESRI));
}