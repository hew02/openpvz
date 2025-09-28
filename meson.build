project('open-pvz', 'cpp',
  default_options: ['cpp_std=c++17'])


deps = [ dependency('ncurses'), dependency('entt') ]
inc = include_directories('include')

executable(
  'openpvz', 
  'src/main.cpp', 
  dependencies : deps, 
  sources : files(
  ),
  include_directories : inc,
  cpp_args : [
    '-Wno-c++11-narrowing'
  ]
)
