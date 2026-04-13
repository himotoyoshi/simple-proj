#ifndef RB_PROJ_H
#define RB_PROJ_H

#include <proj.h>

typedef struct {
  PJ *ref;
  int is_src_latlong;
} Proj;

extern PJ* PJ_DEFAULT_LONGLAT;

extern const rb_data_type_t proj_data_type;

extern VALUE rb_cProj;
extern VALUE rb_cCrs;

VALUE rb_crs_new(PJ *);

#endif