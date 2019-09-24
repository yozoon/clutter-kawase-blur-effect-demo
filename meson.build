project('cluttertest', 'c')

gtk_dep = dependency('gtk+-3.0')
clutter_gtk_dep = dependency('clutter-gtk-1.0')
clutter_dep = dependency('clutter-1.0')
cogl_dep = dependency('cogl-1.0')
cc = meson.get_compiler('c')
m_dep = cc.find_library('m')

configure_file(input : 'baboon.tiff',
  output : 'baboon.tiff',
  copy : true)

sources = ['main.c', 'clutter-kawase-blur-effect.c']
executable('clutterapp', sources, 
    dependencies : [gtk_dep, clutter_gtk_dep, clutter_dep, cogl_dep, m_dep]
)