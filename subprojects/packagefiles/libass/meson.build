project('libass', 'c', 'cpp',
  version: '0.16.0',
  meson_version: '>=0.57.0')

# wip, is a make progress, './autogen.sh  && ./configure && make' and optionally (sudo make install)



mod = import('unstable_external_project')
run_command('./autogen.sh')
p = mod.add_project('configure')

libass_dep = p.dependency('libass')