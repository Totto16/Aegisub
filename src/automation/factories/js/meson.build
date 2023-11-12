
general_inc += include_directories('.')

aegisub_src += files(
    'auto4_js.cpp',
    'auto4_js_assfile.cpp',
    'auto4_js_dialog.cpp',
    'auto4_js_progresssink.cpp',
    'convertable.cpp',
    'v8_helper.cpp',
)



npm = find_program('npm')

repo_dir = meson.current_source_dir() / 'idl-generator'

npm_install = custom_target(
    'npm_install',
    command: [
        npm,
        '--prefix', repo_dir,
        'install',
        '--include=dev',
    ],
    capture: true,
    output: 'npm_install.log',
)

idl_generator_ts_files = []
subdir('idl-generator/src')

npm_build = custom_target(
    'npm_build',
    command: [
        npm,
        '--prefix', repo_dir,
        'run',
        'build',
    ],
    depends: npm_install,
    capture: true,
    output: 'npm_build.log',
    depend_files: idl_generator_ts_files,
)

idl_target = custom_target(
    'idl_headers.h',
    command: [
        npm,
        '--prefix', repo_dir,
        'run',
        'execute',
        '--', 'cpp',
        '@INPUT0@',
        '--output', meson.current_source_dir() / 'idl_headers.h',
    ],
    input: [meson.current_source_dir() / 'idl' / 'aegisub.idl'],
    output: 'idl_headers.h',
)

aegisub_src += idl_target



