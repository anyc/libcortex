project('libcortex', 'c',
  version: '0.7.0',
  meson_version: '>=0.60.0',
  default_options: ['warning_level=1']
)

cc = meson.get_compiler('c')
libdir = get_option('libdir')
plugindir = join_paths(get_option('prefix'), libdir)

subdir('src')
subdir('layer2')

if get_option('build_tests')
  subdir('tests')
endif

subdir('examples')
