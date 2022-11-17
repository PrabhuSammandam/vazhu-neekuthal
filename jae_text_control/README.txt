This is the port of scintill for wxWidgets GUI toolkit.

This port depends on the scintilla release and Lexer release.

If you want to update to latest scintill version do the following.

From scintill source following are the dependent folders for compiling this wxWidgets port.
1. call
2. include
3. src

In the new release of scintilla a new source file added then it need to added in the meson.build file also. Always monitor the above mentioned
folders for any change like file addition or removal.

refer the meson_scintilla_template.build for scintilla meson build file.
