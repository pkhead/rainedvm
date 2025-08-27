project(
    'rainedvm',
    'cpp', 'c',
    version : '2.0.0',
    default_options : ['warning_level=3', 'cpp_std=c++17', 'default_library=static']
)

cxx = meson.get_compiler('cpp')
sources = []
incdirs = []
deps = []

sources += files(
    'src/sys.cpp',

    'src/vm/vm.cpp',
    'src/vm/archive.cpp',
    'src/vm/util.cpp',

    'src/ui/app.cpp',
    'src/ui/md_parser.cpp',

    'deps/md4c/src/entity.c',
    'deps/md4c/src/md4c.c',
    'deps/md4c/src/md4c-html.c',
)

incdirs += include_directories(
    'deps/md4c/src'
)

if target_machine.system() == 'windows'
    deps += dependency('libcurl', static : true, required : true)
endif

deps += dependency('cpr', static : true, required : true)
deps += dependency('zlib', required : true)
deps += dependency('bzip2', required : true)
deps += dependency('liblzma', required : true)
deps += dependency('minizip', required : true)

# if building with gcc on windows, make sure to statically link mingw libraries
link_args = []
if (get_option('buildtype') == 'release' 
        and meson.get_compiler('cpp').get_argument_syntax() == 'gcc'
        and target_machine.system() == 'windows')
    link_args += '-static'
endif

# debug define
cpp_args = ['-DRAINEDUPDATE_VERSION="@0@"'.format(meson.project_version())]
if get_option('buildtype') == 'debug'
    cpp_args += '-DDEBUG'
endif

# get wxWidgets dependency
# wxconfig is required if you are not on windows
wxconfig = find_program('wx-config',
    required : target_machine.system() != 'windows')
wx_incdirs = []
wx_compile_args = []

# use wxconfig
if wxconfig.found()
    deps += dependency('wxwidgets',
        required : true,
        version : '>=3.2.4',
        static : get_option('buildtype') == 'release',
        modules: ['base', 'core', 'html'])
    
    run_res = run_command(wxconfig, '--cflags',
        check : true)
    wx_compile_args += run_res.stdout().strip().split(' ')

# use manual wxwidgets_libdir/includedir options
else
    libdir = get_option('wxwidgets_libdir')
    wx_incdirs += [
        get_option('wxwidgets_includedir'),
    ]

    if target_machine.system() == 'windows'
        wx_compile_args += ['-D__WXMSW__', '-D_UNICODE']
    endif

    if cxx.get_id() != 'msvc'
        if get_option('debug')
            wx_incdirs += join_paths(get_option('wxwidgets_libdir'), 'mswud')
        else
            wx_incdirs += join_paths(get_option('wxwidgets_libdir'), 'mswu')
        endif

        deps += declare_dependency(
            dependencies: [
                cxx.find_library('wxmsw32u_html', dirs : [libdir], required : true),
                cxx.find_library('wxmsw32u_core', dirs : [libdir], required : true),
                cxx.find_library('wxmsw32u_richtext', dirs : [libdir], required : true),
                cxx.find_library('wxbase32u', dirs : [libdir], required : true),
            ],
            include_directories: wx_incdirs,
            compile_args: wx_compile_args
        )
    else
        wx_incdirs = [join_paths(get_option('wxwidgets_includedir'), 'msvc')] + wx_incdirs
        deps += declare_dependency(
            include_directories: wx_incdirs,
            compile_args: wx_compile_args,
            link_args: ['-L' + get_option('wxwidgets_libdir')]
        )
    endif
endif

# build resource file
if target_machine.system() == 'windows'
    windows = import('windows')
    sources += windows.compile_resources('src/ui/winres.rc',
        args: wx_compile_args,
        include_directories: wx_incdirs)
endif

executable('rainedvm',
    sources,
    include_directories : incdirs,
    dependencies : deps,
    win_subsystem : 'windows',
    cpp_args : cpp_args,
    link_args : link_args,
)
