project_cargs = [
  '-D_FILE_OFFSET_BITS=64',
  '-DDEFAULT_EVLOOP="epoll"',
  '-DSO_VERSION="0.7"',
  '-DMAJOR_VERSION=0',
  '-DMINOR_VERSION=7',
  '-DREVISION=0',
]
threads_dep = dependency('threads')
jsonc_dep = dependency('json-c')
systemd_dep = dependency('libsystemd')

static_modules = get_option('static_modules')
static_tools = get_option('tools')
if 'threads' not in static_tools
  error('the threads tool is required by the core library')
endif
static_defines = ''
foreach module : static_modules
  static_defines += '#define CRTX_HAVE_MOD_' + module.to_upper() + ' 1\n'
endforeach

config_h = configure_file(
  input: 'config.h.in',
  output: 'meson_config.h',
  configuration: {'STATIC_MODULE_DEFINES': static_defines}
)
static_includes = ''
static_module_entries = ''
static_listener_entries = ''
foreach module : ['threads', 'signals', 'epoll'] + static_modules + static_tools
  static_includes += '#include "' + module + '.h"\n'
endforeach
foreach module : ['signals', 'epoll'] + static_modules
  static_module_entries += '  {"' + module + '", &crtx_' + module + '_init, &crtx_' + module + '_finish},\n'
  static_listener_entries += '  {"' + module + '", &crtx_setup_' + module + '_listener},\n'
endforeach
core_modules_h = configure_file(
  input: 'core_modules.h.in',
  output: 'meson_core_modules.h',
  configuration: {
    'STATIC_INCLUDES': static_includes,
    'STATIC_MODULE_ENTRIES': static_module_entries,
    'STATIC_LISTENER_ENTRIES': static_listener_entries,
  }
)

core_sources = files('core.c', 'dict.c', 'llist.c', 'evloop.c', 'signals.c', 'epoll.c')
foreach module : static_modules + static_tools
  core_sources += files(module + '.c')
endforeach

inc = include_directories('.')
dllist_lib = static_library('crtx_dllist', files('llist.c'),
  include_directories: inc,
  c_args: project_cargs + ['-DCRTX_DLL'],
  pic: true
)

core_lib = shared_library('crtx', core_sources,
  include_directories: inc,
  link_with: dllist_lib,
  dependencies: [threads_dep, jsonc_dep, systemd_dep],
  c_args: project_cargs + ['-DCRTX_MESON_BUILD', '-DCRTX_PLUGIN_DIR="' + plugindir + '"'],
  version: '0.7.0',
  soversion: '0.7',
  install: true,
  install_dir: libdir
)

install_headers(files('avahi.h', 'can.h', 'core.h', 'curl.h', 'dict.h',
  'dict_inout.h', 'dict_inout_json.h', 'epoll.h', 'evdev.h', 'evloop.h',
  'fanotify.h', 'fork.h', 'inotify.h', 'libnl.h', 'libvirt.h', 'mqtt.h',
  'netlink_ge.h', 'netlink_raw.h', 'nf_queue.h', 'nl_route_raw.h', 'pipe.h',
  'popen.h', 'pulseaudio.h', 'readline.h', 'sd_journal.h', 'sdbus.h',
  'signals.h', 'sm.h', 'threads.h', 'timer.h', 'udev.h', 'uevents.h',
  'uio.h', 'writequeue.h'), subdir: 'cortex')

pkg = import('pkgconfig')
pkg.generate(core_lib,
  name: 'libcortex',
  description: 'Event loop and event-source library',
  version: meson.project_version(),
  subdirs: 'cortex'
)

module_deps = {
  'sdbus': [systemd_dep],
  'avahi': [systemd_dep],
  'can': [dependency('libnl-route-3.0'), dependency('libcap-ng')],
  'curl': [dependency('libcurl')],
  'evdev': [dependency('libevdev')],
  'libnl': [dependency('libnl-3.0')],
  'libvirt': [dependency('libvirt')],
  'netlink_ge': [dependency('libnl-genl-3.0')],
  'pulseaudio': [dependency('libpulse')],
  'readline': [dependency('readline')],
  'sd_journal': [systemd_dep],
  'udev': [dependency('libudev')],
  'v4l': [],
  'mqtt': [dependency('libmosquitto')],
}

nfqueue_dep = cc.find_library('netfilter_queue', required: false)
nfnetlink_dep = cc.find_library('nfnetlink', required: false)
if nfqueue_dep.found() and nfnetlink_dep.found()
  module_deps += {'nf_queue': [nfqueue_dep, nfnetlink_dep]}
endif

module_targets = {}
module_names = get_option('modules')
foreach name : module_names
  module_links = [core_lib]
  if name == 'avahi'
    module_links += [module_targets['sdbus']]
  endif
  module_targets += {name: shared_library('crtx_' + name, files(name + '.c'),
    include_directories: inc,
    link_with: module_links,
    dependencies: module_deps[name],
    c_args: project_cargs,
    name_prefix: 'lib',
    version: '0.7.0',
    soversion: '0.7',
    install: true,
    install_dir: libdir
  )}
endforeach
