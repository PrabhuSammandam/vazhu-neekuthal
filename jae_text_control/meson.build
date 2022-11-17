jae_text_control_srcs=files([
  # ported files
  'jaePortListBox.cpp',
  'jaePortFont.cpp',
  'jaePortSurface.cpp',
  'jaePortMenu.cpp',
  'jaePortWindow.cpp',
  'jaePortPlatform.cpp',

  'jaeUtils.cpp',
  'jaeTextEditControlBase.cpp',
  'jaeListBox.cpp',
  'jaeScintilla.cpp',
  'jaeTextEditControlEvent.cpp',
])

jae_text_control_lib = static_library('jae_text_control',
                                      jae_text_control_srcs,
                                      include_directories: [root_inc],
                                      dependencies : [wxwidget_dep, libscintilla_dep])
