project(
    'Rained.Update',
    'cpp',
    version : '1.0.0',
    default_options : ['warning_level=3', 'cpp_std=c++17', 'default_library=static']
)

sources = files(
    'src/main.cpp',
    'src/sys.cpp',
    'src/app.cpp',

    # imgui sources
    'imgui/imgui_demo.cpp',
    'imgui/imgui_draw.cpp',
    'imgui/imgui_tables.cpp',
    'imgui/imgui_widgets.cpp',
    'imgui/imgui.cpp',
    'imgui/backends/imgui_impl_glfw.cpp',
    'imgui/backends/imgui_impl_opengl3.cpp',
    'imgui/misc/freetype/imgui_freetype.cpp',
)

incdirs = include_directories('imgui')
deps = []

deps += dependency('freetype2', static : host_machine.system() == 'windows')
deps += dependency('glfw3', static : true) # TODO: this isn't statically linking on linux...
deps += dependency('cpr', static : host_machine.system() == 'windows')
deps += dependency('gl')

link_args = []
if get_option('buildtype') == 'release' and meson.get_compiler('cpp').get_argument_syntax() == 'gcc' and host_machine.system() == 'windows'
    link_args += '-static'
endif

cpp_args = ['-DRAINEDUPDATE_VERSION="@0@"'.format(meson.project_version())]
if get_option('buildtype') == 'debug'
    cpp_args += '-DDEBUG'
endif

executable('Rained.Update',
    sources,
    include_directories : incdirs,
    dependencies : deps,
    win_subsystem : 'windows',
    cpp_args : cpp_args,
    link_args : link_args
)
