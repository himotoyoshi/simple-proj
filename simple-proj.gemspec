
Gem::Specification::new do |s|
  version = "1.0.0"

  files = Dir.glob("**/*") - [ 
                               Dir.glob("simple-proj-*.gem"), 
                               Dir.glob("doc/**/*"), 
                               Dir.glob("examples/**/*"), 
                             ].flatten

  s.platform    = Gem::Platform::RUBY
  s.name        = "simple-proj"
  s.summary     = "Extension library for PROJ 7"
  s.description = <<-HERE
    Extension library for PROJ 7
  HERE
  s.version     = version
  s.licenses    = ['MIT']
  s.author      = "Hiroki Motoyoshi"
  s.email       = ""
  s.homepage    = 'https://github.com/himotoyoshi/simple-proj'
  s.files       = files
  s.extensions  = [ "ext/extconf.rb" ]
  s.required_ruby_version = ">= 1.8.1"
end
