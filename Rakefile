task :default => :test
desc 'test'
task :test do
  exec 'ruby test/test.rb'
end
desc 'generate mrb for regex'
task :codegen do
  exec 'ruby util/regexpcodegen.rb'
end
