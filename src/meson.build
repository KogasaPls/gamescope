dep_xdamage = dependency('xdamage')
dep_xcomposite = dependency('xcomposite')
dep_xcursor = dependency('xcursor')
dep_xrender = dependency('xrender')
dep_xext = dependency('xext')
dep_xfixes = dependency('xfixes')
dep_xxf86vm = dependency('xxf86vm')
dep_xtst = dependency('xtst')
dep_xres = dependency('xres')
dep_xmu = dependency('xmu')
dep_xi = dependency('xi')

drm_dep = dependency('libdrm', version: '>= 2.4.113', required: get_option('drm_backend'))
eis_dep = dependency('libeis-1.0', required : get_option('input_emulation'))

wayland_server = dependency('wayland-server', version: '>=1.21')
xkbcommon = dependency('xkbcommon')
thread_dep = dependency('threads')
cap_dep = dependency('libcap', required: get_option('rt_cap'))
epoll_dep = dependency('epoll-shim', required: false)
sdl2_dep = dependency('SDL2', required: get_option('sdl2_backend'))
avif_dep = dependency('libavif', version: '>=1.0.0', required: get_option('avif_screenshots'))
pixman_dep = dependency('pixman-1')
udev_dep = dependency('libudev')

wlroots_dep = dependency(
  'wlroots-0.20',
  version: ['>= 0.20.0', '< 0.21.0'],
  fallback: 'wlroots',
  default_options: ['default_library=static', 'examples=false', 'xwayland=enabled', 'backends=libinput', 'renderers=[]', 'allocators=[]', 'session=enabled'],
)

displayinfo_dep = dependency(
  'libdisplay-info',
  version: ['>= 0.0.0', '< 0.5.0'],
  fallback: 'libdisplay-info',
  default_options: ['default_library=static'],
)

libdecor_dep = dependency('libdecor-0')

glsl_compiler = find_program('glslang', 'glslangValidator', native: true)

# Use --depfile to rebuild shaders when included files have changed. Sadly debian based
# distros don't have up-to-date glslang so we need to check for support first.
if run_command(glsl_compiler, ['--version', '--depfile', 'dummy.dep'], check: false).returncode() == 0
  glsl_generator = generator(
    glsl_compiler,
    output    : ['@BASENAME@.h'],
    arguments : ['-V', '@INPUT@', '--vn', '@BASENAME@', '-o', '@OUTPUT@', '--depfile', '@DEPFILE@', '--quiet'],
    depfile   : '@BASENAME@.h.d',
  )
else
  glsl_generator = generator(
    glsl_compiler,
    output    : ['@BASENAME@.h'],
    arguments : ['-V', '@INPUT@', '--vn', '@BASENAME@', '-o', '@OUTPUT@'],
  )
endif

shader_src = [
  'shaders/cs_composite_blit.comp',
  'shaders/cs_composite_blur.comp',
  'shaders/cs_composite_blur_cond.comp',
  'shaders/cs_composite_rcas.comp',
  'shaders/cs_easu.comp',
  'shaders/cs_easu_fp16.comp',
  'shaders/cs_gaussian_blur_horizontal.comp',
  'shaders/cs_nis.comp',
  'shaders/cs_nis_fp16.comp',
  'shaders/cs_rgb_to_nv12.comp',
]

spirv_shaders = glsl_generator.process(shader_src)

reshade_src = [
  'reshade/source/effect_codegen_spirv.cpp',
  'reshade/source/effect_expression.cpp',
  'reshade/source/effect_lexer.cpp',
  'reshade/source/effect_parser_exp.cpp',
  'reshade/source/effect_parser_stmt.cpp',
  'reshade/source/effect_preprocessor.cpp',
  'reshade/source/effect_symbol_table.cpp',
]

reshade_include = include_directories([
  'reshade/source',
  'reshade/include',
  '../thirdparty/SPIRV-Headers/include/spirv/unified1'
])

sol2_include = include_directories(['../thirdparty'])

required_wlroots_features = ['xwayland']

src = [
  'Backends/HeadlessBackend.cpp',
  'Backends/WaylandBackend.cpp',
  'Utils/DirHelpers.cpp',
  'Utils/TempFiles.cpp',
  'Utils/Version.cpp',
  'Utils/Process.cpp',
  'Script/Script.cpp',
  'BufferMemo.cpp',
  'steamcompmgr.cpp',
  'convar.cpp',
  'convar_script.cpp',
  'commit.cpp',
  'main.cpp',
  'edid.cpp',
  'wlserver.cpp',
  'vblankmanager.cpp',
  'rendervulkan.cpp',
  'log.cpp',
  'ime.cpp',
  'mangoapp.cpp',
  'Timeline.cpp',
  'reshade_effect_manager.cpp',
  'backend.cpp',
  'x11cursor.cpp',
  'InputEmulation.cpp',
  'LibInputHandler.cpp',
]

luajit_dep = dependency( 'luajit' )
libinput_dep = dependency('libinput', required: true)

gamescope_cpp_args = []
if drm_dep.found()
  src += 'Backends/DRMBackend.cpp'
  src += 'modegen.cpp'
  required_wlroots_features += 'libinput_backend'
  liftoff_dep = dependency(
    'libliftoff',
    version: ['>= 0.5.0', '< 0.6.0'],
    fallback: 'libliftoff',
    default_options: ['default_library=static'],
  )
else
  liftoff_dep = dependency('', required: false)
endif

if sdl2_dep.found()
  src += 'Backends/SDLBackend.cpp'
endif

gamescope_cpp_args += '-DHAVE_DRM=@0@'.format(drm_dep.found().to_int())
gamescope_cpp_args += '-DHAVE_SDL2=@0@'.format(sdl2_dep.found().to_int())
gamescope_cpp_args += '-DHAVE_AVIF=@0@'.format(avif_dep.found().to_int())
gamescope_cpp_args += '-DHAVE_LIBCAP=@0@'.format(cap_dep.found().to_int())
gamescope_cpp_args += '-DHAVE_LIBEIS=@0@'.format(eis_dep.found().to_int())
gamescope_cpp_args += '-DHAVE_SCRIPTING=1'

src += spirv_shaders
src += protocols_server_src
src += protocols_client_src

if pipewire_dep.found()
  src += 'pipewire.cpp'
endif

if openvr_dep.found()
  src += 'Backends/OpenVRBackend.cpp'
endif

foreach feat : required_wlroots_features
  if wlroots_dep.get_variable('have_' + feat) != 'true'
    error('Cannot use wlroots built without ' + feat + ' support')
  endif
endforeach

cc = meson.get_compiler('c')
compiler_name = cc.get_id()
compiler_version = cc.version()

vcs_tag_cmd = ['git', 'describe', '--always', '--tags', '--dirty=+']
vcs_tag = run_command(vcs_tag_cmd, check: false).stdout().strip()
version_tag = vcs_tag + ' (' + compiler_name + ' ' + compiler_version + ')'

# color_helpers.cpp is the one translation unit that wants fast math: the
# colour-management LUT builders (calcColorTransform and friends). Give it the
# flag per-file rather than wrapping the code in
# `#pragma GCC optimize("-ffast-math")`. The pragma gives every function inside
# the region a different optimization node from its callers, and GCC's inliner
# will not cross that boundary ("optimization level attribute mismatch"), which
# costs far more than fast math wins.
#
# -fno-lto is deliberate and load-bearing. Fast math is an Optimization flag, so
# under LTO the mismatch between this TU's nodes and every other TU's reappears
# at the IPA stage and eats most of the win back. Compiling this one TU without
# LTO keeps its intra-TU optimisation intact; nothing here is on a per-frame
# path (the LUT rebuild is dirty-checked and only runs on a colour-management
# change), so losing cross-TU inlining for it costs nothing measurable.
#
# One full colour-management LUT rebuild, SDR/HDR output, znver5 -O3 -flto=auto,
# GCC 16.1.1, medians of 150 warmed iterations:
#
#     upstream, global -ffast-math                 0.897 / 0.596 ms
#     no fast math anywhere                        1.269 / 0.836 ms
#     PR 1494 as written (pragma)                  1.905 / 1.433 ms
#     this TU -ffast-math, still in LTO            1.264 / 1.026 ms
#     this TU -ffast-math -fno-lto  (what we do)   1.026 / 0.624 ms
color_helpers_lib = static_library(
  'color_helpers',
  'color_helpers.cpp',
  dependencies: [glm_dep],
  # gamescope_cpp_args carries the HAVE_* feature defines; without it a future
  # #if HAVE_DRM (etc.) in color_helpers would silently compile the wrong
  # branch here while every other TU sees the right one.
  cpp_args: gamescope_cpp_args + ['-ffast-math', '-fno-lto'],
)

gamescope_version_conf = configuration_data()
gamescope_version_conf.set('VCS_TAG', version_tag)

gamescope_version = configure_file(
  input : 'GamescopeVersion.h.in',
  output : 'GamescopeVersion.h',
  configuration : gamescope_version_conf
)

  executable(
    'gamescope',
    src, reshade_src, gamescope_version,
    include_directories : [reshade_include, sol2_include],
    dependencies: [
      dep_wayland, dep_x11, dep_xdamage, dep_xcomposite, dep_xrender, dep_xext, dep_xfixes,
      dep_xxf86vm, dep_xres, glm_dep, drm_dep, wayland_server,
      xkbcommon, thread_dep, sdl2_dep, wlroots_dep,
      vulkan_dep, liftoff_dep, dep_xtst, dep_xmu, cap_dep, epoll_dep, pipewire_dep, librt_dep,
      stb_dep, displayinfo_dep, openvr_dep, dep_xcursor, avif_dep, dep_xi,
      libdecor_dep, eis_dep, luajit_dep, libinput_dep, pixman_dep, udev_dep,
    ],
    link_with: color_helpers_lib,
    install: true,
    cpp_args: gamescope_cpp_args,
  )

gamescope_core_src = [
  'convar.cpp',
  'log.cpp',
  'Utils/DirHelpers.cpp',
  'Utils/Process.cpp',
  'Utils/Version.cpp',
]

if pipewire_dep.found()
  executable( 'gamescopestream', ['Apps/gamescopestream.cpp'], gamescope_core_src, gamescope_version, protocols_client_src, dependencies: [ pipewire_dep, dep_wayland, libdecor_dep, cap_dep ], install: true )
endif

executable('gamescopereaper', ['Apps/gamescopereaper.cpp', gamescope_core_src], gamescope_version, dependencies: [cap_dep], install:true )

benchmark_dep = dependency('benchmark', required: get_option('benchmark'), disabler: true)
executable('gamescope_color_microbench', ['color_bench.cpp'], gamescope_core_src, gamescope_version, link_with: color_helpers_lib, dependencies:[benchmark_dep, glm_dep, cap_dep])

executable('gamescope_color_tests', ['color_tests.cpp'], gamescope_core_src, gamescope_version, link_with: color_helpers_lib, dependencies:[glm_dep, cap_dep])

executable('gamescopectl', ['Apps/gamescopectl.cpp'], gamescope_core_src, gamescope_version, protocols_client_src, dependencies: [dep_wayland, cap_dep], install:true )

executable('gamescope-type', ['Apps/gamescope_type.c'], protocols_client_src, dependencies: [dep_wayland, cap_dep], install:true )

executable('gamescope_hotkey_example', ['Apps/gamescope_hotkey_example.cpp'], gamescope_core_src, gamescope_version, protocols_client_src, dependencies: [dep_wayland, xkbcommon, cap_dep], install: false )
