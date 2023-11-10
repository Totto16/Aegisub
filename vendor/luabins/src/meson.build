luabins_src = files(
    'fwrite.c',
    'load.c',
    'luabins.c',
    'luainternals.c',
    'save.c',
    'savebuffer.c',
    'write.c',
)

libluabins = static_library(
    'luabins',
    luabins_src,
    include_directories: luajit_inc,
    dependencies: lua_dep,
)
