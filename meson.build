project('vazhu-neeguthal', 'c','cpp',
       default_options : ['cpp_std=c++17', 'buildtype=debug'])

wxwidget_dep=dependency('wxwidgets', modules:['core', ' base', 'aui', 'stc'])
ccomp = meson.get_compiler('c')
m_dep = ccomp.find_library('util', required : false)

liblexilla_proj = subproject('lexilla')
liblexilla_dep  = liblexilla_proj.get_variable('liblexilla_dep')

libscintilla_proj = subproject('scintilla')
libscintilla_dep  = libscintilla_proj.get_variable('libscintilla_dep')

srcs=files([
  'main.cpp',
  'MainFrame.cpp',
  'MainFrame_ICore.cpp',
  'MainFrame_Session.cpp',
  'GdbMgr.cpp',
  'asyncprocess.cpp',
  'processreaderthread.cpp',
  'unixprocess_impl.cpp',
  'StringUtils.cpp',
  'CustomEvents.cpp',
  'GdbResultTree.cpp',
  'GdbModels.cpp',
  'GdbParser.cpp',
  'jaeCppTextDocument.cpp',
  'gdbmiparser.cpp',
  'cl_treelistctrl.cpp',
  'drawingutils.cpp',
  'clColours.cpp',
  'event_notifier.cpp',
  'clSystemSettings.cpp',
  'ui/StackPanel.cpp',
  'ui/BreakPointsPanel.cpp',
  'ui/LocalVariablesPanel.cpp',
  'ui/NewSimpleBreakpointDialog.cpp',
  'ui/SessionPanel.cpp',
  'ui/ExecSessionDialog.cpp',
  'ui/RemoteAttachSessionDialog.cpp'
])

root_inc        = include_directories('.')

subdir('jae_text_control')

executable('vazhu-neeguthal',
           srcs,
           include_directories : [root_inc],
           dependencies: [wxwidget_dep, m_dep, liblexilla_dep, libscintilla_dep],
           link_with : [jae_text_control_lib]
           )

source = join_paths(meson.source_root(), 'resource')
dest = join_paths(meson.build_root(), 'resource')
message('copying @0@ to @1@ ...'.format(source, dest))
r = run_command('cp', '-R' ,source, dest)
