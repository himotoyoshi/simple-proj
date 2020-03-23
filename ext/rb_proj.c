#include "ruby.h"
#include "rb_proj.h"

VALUE rb_cProj;

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

If two arguments are given, the first is the source CRS definition and the
second is the target CRS definition. If only one argument is given, the
following two cases are possible. 

 * a proj-string, which represents a transformation. 
 * a CRS defintion, in which case the latlong coordinates are implicitly 
   used as the source CRS definition.

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

*/

static VALUE
rb_proj_initialize (int argc, VALUE *argv, VALUE self)
{
  volatile VALUE vdef1, vdef2;
  Proj *proj;
  PJ *ref, *src, *tgt;
  PJ_TYPE type;
  int errno;

  rb_scan_args(argc, argv, "11", &vdef1, &vdef2);

  Data_Get_Struct(self, Proj, proj);

  if ( NIL_P(vdef2) ) {
    Check_Type(vdef1, T_STRING);
    ref = proj_create(PJ_DEFAULT_CTX, StringValuePtr(vdef1));
    if ( proj_is_crs(ref) ) {
      proj_destroy(ref);
      ref = proj_create_crs_to_crs(PJ_DEFAULT_CTX, "+proj=latlong +type=crs", StringValuePtr(vdef1), NULL);    
    }
    proj->ref = ref;
    proj->is_src_latlong = 1;
  }
  else {
    Check_Type(vdef1, T_STRING);
    Check_Type(vdef2, T_STRING);
    ref = proj_create_crs_to_crs(PJ_DEFAULT_CTX, StringValuePtr(vdef1), StringValuePtr(vdef2), NULL);    
    src = proj_get_source_crs(PJ_DEFAULT_CTX, ref);
    type = proj_get_type(src);
    proj->ref = ref;
    proj->is_src_latlong = 0;      
  }
  
  if ( ! ref ) {
    errno = proj_context_errno(PJ_DEFAULT_CTX);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }
  
  return Qnil;
}

/*
Returns source CRS as PROJ object.

@return [PROJ, nil]
*/
static VALUE
rb_proj_source_crs (VALUE self)
{
  volatile VALUE vout;
  Proj *proj, *out;
  PJ *crs;
  const char *wkt;
  int errno;

  Data_Get_Struct(self, Proj, proj);
  crs = proj_get_source_crs(PJ_DEFAULT_CTX, proj->ref);

  if ( ! crs ) {
    return Qnil;
  }
  
  vout = rb_proj_s_allocate(rb_cProj);
  Data_Get_Struct(vout, Proj, out);
  
  out->ref = crs;

  return vout;
}

/*
Returns target CRS as PROJ object.

@return [PROJ, nil]
*/
static VALUE
rb_proj_target_crs (VALUE self)
{
  volatile VALUE vout;
  Proj *proj, *out;
  PJ *crs;
  const char *wkt;
  int errno;

  Data_Get_Struct(self, Proj, proj);
  crs = proj_get_target_crs(PJ_DEFAULT_CTX, proj->ref);

  if ( ! crs ) {
    return Qnil;
  }
  
  vout = rb_proj_s_allocate(rb_cProj);
  Data_Get_Struct(vout, Proj, out);
  
  out->ref = crs;

  return vout;
}

/*
Returns WKT expression of CRS definition.

@return [String]
*/
static VALUE
rb_proj_as_wkt (VALUE self)
{
  Proj *proj;
  const char *wkt;

  Data_Get_Struct(self, Proj, proj);
  wkt = proj_as_wkt(PJ_DEFAULT_CTX, proj->ref, PJ_WKT2_2015_SIMPLIFIED, NULL);
  if ( ! wkt ) {
    return Qnil;
  }
  return rb_str_new2(wkt);
}

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

static VALUE
rb_proj_as_proj_json (VALUE self)
{
  Proj *proj;
  const char *json;

  Data_Get_Struct(self, Proj, proj);
  json = proj_as_projjson(PJ_DEFAULT_CTX, proj->ref, NULL);
  if ( ! json ) {
    return Qnil;
  }
  return rb_str_new2(json);
}

static VALUE
rb_proj_definition (VALUE self)
{
  Proj *proj;
  PJ_PROJ_INFO info;

  Data_Get_Struct(self, Proj, proj);
  info = proj_pj_info(proj->ref);
  return rb_str_new2(info.definition);
}

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
Transforms coordinates forwardly from (lat1, lon1, z1) to (x1, y2, z2).
The order of coordinates arguments should be longitude, latitude, and height.
The input longitude and latitude should be in units 'degrees'.

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

  rb_scan_args(argc, argv, "21", &vlon, &vlat, &vz);

  Data_Get_Struct(self, Proj, proj);

  if ( ! proj->is_src_latlong ) {
    rb_raise(rb_eRuntimeError, "requires latlong src crs. use #transform_forward instead of #forward.");
  }

  data_in.lpz.lam = proj_torad(NUM2DBL(vlon));
  data_in.lpz.phi = proj_torad(NUM2DBL(vlat));
  data_in.lpz.z   = NIL_P(vz) ? 0.0 : NUM2DBL(vz);

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

  data_in.xyz.x = NUM2DBL(vx);
  data_in.xyz.y = NUM2DBL(vy);
  data_in.xyz.z = NIL_P(vz) ? 0.0 : NUM2DBL(vz);

  data_out = proj_trans(proj->ref, PJ_INV, data_in);

  if ( data_out.lpz.lam == HUGE_VAL ) {
    errno = proj_errno(proj->ref);
    rb_raise(rb_eRuntimeError, "%s", proj_errno_string(errno));
  }

  if ( NIL_P(vz) ) {
    return rb_assoc_new(rb_float_new(proj_todeg(data_out.lpz.lam)), 
                        rb_float_new(proj_todeg(data_out.lpz.phi)));
  } else {
    return rb_ary_new3(3, rb_float_new(proj_todeg(data_out.lpz.lam)), 
                          rb_float_new(proj_todeg(data_out.lpz.phi)),
                          rb_float_new(data_out.lpz.z));
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

void
Init_simple_proj ()
{
  rb_cProj = rb_define_class("PROJ", rb_cObject);

  rb_define_alloc_func(rb_cProj, rb_proj_s_allocate);
  rb_define_method(rb_cProj, "initialize", rb_proj_initialize, -1);

  rb_define_method(rb_cProj, "source_crs", rb_proj_source_crs, 0);
  rb_define_method(rb_cProj, "target_crs", rb_proj_target_crs, 0);

  rb_define_method(rb_cProj, "to_wkt", rb_proj_as_wkt, 0);
  rb_define_method(rb_cProj, "to_string", rb_proj_as_proj_string, 0);
  rb_define_method(rb_cProj, "to_json", rb_proj_as_proj_json, 0);
  rb_define_private_method(rb_cProj, "_definition", rb_proj_definition, 0);
  rb_define_method(rb_cProj, "ellipsoid_get_parameters", rb_proj_ellipsoid_get_parameters, 0);

  rb_define_method(rb_cProj, "forward", rb_proj_forward, -1);
  rb_define_method(rb_cProj, "inverse", rb_proj_inverse, -1);

  rb_define_method(rb_cProj, "transform", rb_proj_transform_forward, -1);
  rb_define_method(rb_cProj, "transform_inverse", rb_proj_transform_inverse, -1);

}