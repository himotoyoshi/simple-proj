
Gem::Specification::new do |s|
  version = "1.0.1"

  files = Dir.glob("**/*") - [ 
                               Dir.glob("simple-proj-*.gem"), 
                               Dir.glob("doc/**/*"), 
                               Dir.glob("examples/**/*"), 
                             ].flatten

  s.platform    = Gem::Platform::RUBY
  s.name        = "simple-proj"
  s.summary     = "Ruby extension library for PROJ 7"
  s.description = <<-HERE
    Ruby extension library for PROJ 7
  HERE
  s.version     = version
  s.license     = 'MIT'
  s.author      = "Hiroki Motoyoshi"
  s.email       = ""
  s.homepage    = 'https://github.com/himotoyoshi/simple-proj'
  s.files       = files
  s.extensions  = [ "ext/extconf.rb" ]
  s.required_ruby_version = ">= 2.4.0"
  s.add_runtime_dependency 'bindata', '~> 2.4', '>= 2.4.8'
end
