require "mkmf"
require 'carray/mkmf'
require "open-uri"

dir_config("proj", possible_includes, possible_libs)

if have_header("proj.h") and have_library("proj")
  have_carray()
  create_makefile("simple_proj")
end


