@echo off

set COMMON_COMPILER_FLAGS=/nologo /EHa- /GR- /fp:fast /Oi /W4 /Fm /std:c++17
set DEBUG_FLAGS=/DDEBUG_BUILD /Od /MTd /Zi
set RELEASE_FLAGS=/O2
set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% %DEBUG_FLAGS%
REM set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% %RELEASE_FLAGS%
set LINKER_FLAGS=/INCREMENTAL:NO /opt:ref
set SYSTEM_LIBS=user32.lib gdi32.lib winmm.lib d2d1.lib dwrite.lib dxgi.lib d3dcompiler.lib d3d11.lib
cl %COMPILER_FLAGS% world.cpp /link %LINKER_FLAGS% %SYSTEM_LIBS%

echo Done
