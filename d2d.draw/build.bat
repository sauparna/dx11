@echo off

set COMMON_COMPILER_FLAGS=/nologo /EHsc /GR- /fp:fast /Oi /W4 /Fm /std:c++17
set DEBUG_FLAGS=/DDEBUG_BUILD /Od /MTd /Zi
set PREPROCESSOR_DEFS=/DUNICODE /DNOMINMAX
set RELEASE_FLAGS=/O2
set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% %DEBUG_FLAGS% %PREPROCESSOR_DEFS%
REM set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% %RELEASE_FLAGS%
set LINKER_FLAGS=/INCREMENTAL:NO /opt:ref
set SYSTEM_LIBS=kernel32.lib user32.lib gdi32.lib winmm.lib ole32.lib d2d1.lib dxgi.lib d3d11.lib dwrite.lib
set LOCAL_LIBS=kwindow.lib
set SRC=kdraw.cpp kd2dsurface.cpp kdrawingengine.cpp kscene.cpp
cl %COMPILER_FLAGS% %SRC% /link %LINKER_FLAGS% %SYSTEM_LIBS% %LOCAL_LIBS%
echo Done
