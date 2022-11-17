project('libscintilla', 'cpp',
       default_options : ['cpp_std=c++17'])

srcs=files([
'./call/ScintillaCall.cxx',
'./src/AutoComplete.cxx',
'./src/CallTip.cxx',
'./src/CaseConvert.cxx',
'./src/CaseFolder.cxx',
'./src/CellBuffer.cxx',
'./src/CharClassify.cxx',
'./src/CharacterCategoryMap.cxx',
'./src/CharacterType.cxx',
'./src/ContractionState.cxx',
'./src/DBCS.cxx',
'./src/Decoration.cxx',
'./src/Document.cxx',
'./src/EditModel.cxx',
'./src/EditView.cxx',
'./src/Editor.cxx',
'./src/Geometry.cxx',
'./src/Indicator.cxx',
'./src/KeyMap.cxx',
'./src/LineMarker.cxx',
'./src/MarginView.cxx',
'./src/PerLine.cxx',
'./src/PositionCache.cxx',
'./src/RESearch.cxx',
'./src/RunStyles.cxx',
'./src/ScintillaBase.cxx',
'./src/Selection.cxx',
'./src/Style.cxx',
'./src/UniConversion.cxx',
'./src/UniqueString.cxx',
'./src/ViewStyle.cxx',
'./src/XPM.cxx'
])

libscintilla_inc = include_directories('include', 'src')
libscintilla = static_library('scintill',
                              srcs,
                              include_directories : libscintilla_inc,
                              install : true)

libscintilla_dep = declare_dependency(include_directories : libscintilla_inc,
                                      link_with : libscintilla)
