# libomarchy-gtk: Omarchy UI for GTK 4 (and, through GObject Introspection, Python, GJS, Vala and
# Rust). Built into build/gtk by gtk/build.
project('omarchy-gtk', 'c',
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>= 1.1',
  default_options: ['c_std=gnu11', 'warning_level=2', 'buildtype=debugoptimized'])

api = '1'
fs = import('fs')
gnome = import('gnome')
pkgconfig = import('pkgconfig')
python = find_program('python3')
cc = meson.get_compiler('c')

add_project_arguments(
  cc.get_supported_arguments(['-Wno-unused-parameter', '-Wno-missing-field-initializers', '-Wno-sign-compare',
                              '-Wno-cast-function-type', '-Werror=implicit-function-declaration']),
  language: 'c')

gtk_dep = dependency('gtk4', version: '>= 4.14')
adw_dep = dependency('libadwaita-1', version: '>= 1.6')
json_dep = dependency('json-glib-1.0')
theme_dep = dependency('omarchy-theme')
gio_unix_dep = dependency('gio-unix-2.0')
m_dep = cc.find_library('m', required: false)

# Where libomarchy-theme lives, so uninstalled binaries find it without LD_LIBRARY_PATH.
theme_libdir = theme_dep.get_variable(pkgconfig: 'libdir')

list = {}
foreach what : ['src', 'headers', 'sections', 'tests']
  list += { what: run_command(python, 'gen.py', 'list', what, check: true).stdout().strip().split('\n') }
endforeach

version_h = configure_file(
  output: 'omarchy-version.h',
  configuration: { 'VERSION': meson.project_version() },
  input: 'src/omarchy-version.h.in')

public_headers = files(list['headers'])
umbrella = custom_target('omarchy-gtk.h',
  output: 'omarchy-gtk.h',
  input: public_headers,
  command: [python, files('gen.py'), 'umbrella', '@OUTPUT@', '@INPUT@'],
  install: true,
  install_dir: get_option('includedir') / 'omarchy-gtk-' + api)
types_c = custom_target('omarchy-types.c',
  output: 'omarchy-types.c',
  input: public_headers,
  command: [python, files('gen.py'), 'types', '@OUTPUT@', '@INPUT@'])

deps = [gtk_dep, adw_dep, json_dep, theme_dep, gio_unix_dep, m_dep]
inc = include_directories('src', '.')

lib = shared_library('omarchy-gtk-' + api,
  files(list['src']), umbrella, types_c, version_h,
  dependencies: deps,
  include_directories: inc,
  c_args: ['-DOMARCHY_GTK_COMPILATION', '-DG_LOG_DOMAIN="omarchy-gtk"'],
  gnu_symbol_visibility: 'default',
  version: '0.1.0',
  soversion: '0',
  build_rpath: theme_libdir,
  install: true)

omarchy_gtk_dep = declare_dependency(
  link_with: lib,
  include_directories: inc,
  sources: [umbrella, version_h],
  dependencies: [gtk_dep, adw_dep, json_dep, theme_dep])

install_headers(public_headers, version_h, subdir: 'omarchy-gtk-' + api)

pkgconfig.generate(lib,
  name: 'omarchy-gtk-' + api,
  filebase: 'omarchy-gtk-' + api,
  description: 'Omarchy UI for GTK 4: the live Omarchy theme and the component kit',
  subdirs: 'omarchy-gtk-' + api,
  requires: ['gtk4', 'libadwaita-1', 'json-glib-1.0', 'omarchy-theme'])

# GObject Introspection: Omarchy-1.gir and .typelib, when g-ir-scanner is here.
scanner = find_program('g-ir-scanner', required: get_option('introspection'))
have_gi = scanner.found()
if have_gi
  gir = gnome.generate_gir(lib,
    sources: [public_headers, files(list['src']), umbrella, version_h],
    namespace: 'Omarchy',
    nsversion: api,
    identifier_prefix: 'Omarchy',
    symbol_prefix: 'omarchy',
    export_packages: 'omarchy-gtk-' + api,
    includes: ['Gtk-4.0'],
    header: 'omarchy-gtk.h',
    extra_args: ['-DOMARCHY_GTK_COMPILATION', '--quiet', '--warn-all'],
    dependencies: deps,
    install: true)
  vapigen = find_program('vapigen', required: false)
  if vapigen.found()
    gnome.generate_vapi('omarchy-gtk-' + api,
      sources: gir[0],
      packages: ['gtk4'],
      install: true)
  endif
endif

# The gallery, the tests and the live test: build/gtk/omarchy-gtk-*.
sections_c = custom_target('gallery-sections.c',
  output: 'gallery-sections.c',
  input: files(list['sections']),
  command: [python, files('gen.py'), 'sections', '@OUTPUT@', '@INPUT@'])

support = static_library('omarchy-gtk-support',
  'gallery/headless.c', 'tests/drive.c',
  dependencies: [omarchy_gtk_dep, gtk_dep],
  include_directories: include_directories('gallery', 'tests'))

executable('omarchy-gtk-gallery',
  'gallery/main.c', 'gallery/gallery.c', files(list['sections']), sections_c,
  dependencies: [omarchy_gtk_dep, gtk_dep, m_dep],
  include_directories: include_directories('gallery', 'tests'),
  link_with: support,
  build_rpath: theme_libdir,
  install: true)

executable('omarchy-gtk-headless',
  'gallery/headless-run.c',
  dependencies: [gtk_dep],
  include_directories: include_directories('gallery'),
  link_with: support,
  build_rpath: theme_libdir,
  install: true)

executable('omarchy-gtk-tests',
  'tests/main.c', files(list['tests']),
  dependencies: [omarchy_gtk_dep, gtk_dep, m_dep],
  include_directories: include_directories('gallery', 'tests'),
  link_with: support,
  build_rpath: theme_libdir)

executable('omarchy-gtk-live-test',
  'tests/live.c',
  dependencies: [omarchy_gtk_dep, gtk_dep, m_dep],
  include_directories: include_directories('gallery', 'tests'),
  link_with: support,
  build_rpath: theme_libdir)

summary({ 'introspection': have_gi, 'libomarchy-theme': theme_libdir })
