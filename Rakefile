GEMSPEC = "simple-proj.gemspec"

task :install do
  spec = eval File.read(GEMSPEC)
  system %{
    gem build #{GEMSPEC}; gem install #{spec.full_name}.gem
  }
end

require 'rspec/core/rake_task'
RSpec::Core::RakeTask.new