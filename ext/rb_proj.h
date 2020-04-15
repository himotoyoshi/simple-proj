#ifndef RB_PROJ_H
#define RB_PROJ_H

#include <proj.h>

typedef struct {
  PJ *ref;
  int is_src_latlong;
} Proj;

extern PJ* PJ_DEFAULT_LONGLAT;

extern VALUE rb_cProj;
extern VALUE rb_cCrs;

VALUE rb_crs_new(PJ *);

#endif