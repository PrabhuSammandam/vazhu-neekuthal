project('vazhu-neeguthal', 'c','cpp',
       default_options : ['cpp_std=c++17', 'buildtype=debug'])

flex = find_program('flex', required: false)
bison = find_program('bison', required: false)

if not flex.found()
  error('MESON_SKIP_TEST flex not found.')
endif

if not bison.found()
  error('MESON_SKIP_TEST bison not found.')
endif

wxwidget_dep=dependency('wxwidgets', modules:['core', ' base', 'aui', 'stc'])

ccomp = meson.get_compiler('c')
m_dep = ccomp.find_library('util', required : false)

liblexilla_proj = subproject('lexilla')
liblexilla_dep  = liblexilla_proj.get_variable('liblexilla_dep')

libscintilla_proj = subproject('scintilla')
libscintilla_dep  = libscintilla_proj.get_variable('libscintilla_dep')

gdb_mi_grammer_files = custom_target(
  'GdbMiGrammer',
  output : ['GdbMiParser.cpp', 'GdbMiParser.hpp', 'location.hh'],
  input : 'GdbMiGrammer.g',
  command : [bison, '@INPUT@', '--defines', '--output=@OUTPUT0@'],
)

gdb_mi_lexer_files = custom_target(
  'GdbMiLexer',
  output : ['GdbMiLexer.cpp'],
  input : 'GdbMiLexer.l',
  command : [flex ,'--outfile=@OUTPUT0@', '@INPUT@'],
)

srcs=files([
  'GdbMiProcess.cpp',
  'GdbMgr_BreakPoint.cpp',
  'GdbMgr_Catchpoint.cpp',
  'GdbMgr_ProgramContext.cpp',
  'GdbMgr_ThreadCommands.cpp',
  'GdbMgr_ProgramExecution.cpp',
  'GdbMgr_StackManipulation.cpp',
  'GdbMgr_VariableObjects.cpp',
  'GdbMgr_DataManipulation.cpp',
  'GdbMgr_Tracepoint.cpp',
  'GdbMgr_SymbolQuery.cpp',
  'GdbMgr_File.cpp',
  'GdbMgr_TargetManipulation.cpp',
  'GdbMgr_FileTransferCommands.cpp',
  'GdbMgr_SupportCommands.cpp',
  'GdbMgr_Misc.cpp',
  'GdbMgr.cpp',
  'main.cpp',
  'MainFrame.cpp',
  'MainFrame_ICore.cpp',
  'MainFrame_Session.cpp',
  'CustomEvents.cpp',
  'KeyProcessor.cpp',
  'GdbModels.cpp',
  'jaeCppTextDocument.cpp',
  'cl_treelistctrl.cpp',
  'ui/StackPanel.cpp',
  'ui/BreakPointsPanel.cpp',
  'ui/LocalVariablesPanel.cpp',
  'ui/NewSimpleBreakpointDialog.cpp',
  'ui/SessionPanel.cpp',
  'ui/ExecSessionDialog.cpp',
  'ui/RemoteAttachSessionDialog.cpp',
  'ui/SourceFilesPanel.cpp',
  'ui/SessionOpenDialog.cpp',
  'ui/GoToLineDialog.cpp'
])

root_inc        = include_directories('.')

gdb_mi_parser_lib = static_library('GdbMiParser',
                                     'GdbMiDriver.cpp',
                                     'GdbMiScanner.hpp',
                                     'GdbMiResp.cpp',
                                     'GdbMiResp.h',
                                     [gdb_mi_grammer_files],
                                     [gdb_mi_lexer_files],
                                     include_directories : [root_inc])


subdir('jae_text_control')

executable('vazhu-neeguthal',
           srcs,
           include_directories : [root_inc],
           dependencies: [wxwidget_dep, m_dep, liblexilla_dep, libscintilla_dep],
           link_with : [jae_text_control_lib, gdb_mi_parser_lib]
           )

source = join_paths(meson.source_root(), 'resource')
dest = join_paths(meson.build_root(), 'resource')
message('copying @0@ to @1@ ...'.format(source, dest))
r = run_command('cp', '-R' ,source, dest)
