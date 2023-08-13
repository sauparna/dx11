@echo off

set COMMON_COMPILER_FLAGS=/nologo /EHa- /GR- /fp:fast /Oi /W4 /Fm /std:c++17
set DEBUG_FLAGS=/DDEBUG_BUILD /Od /MTd /Zi
set RELEASE_FLAGS=/O2
set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% %DEBUG_FLAGS%
REM set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS% %RELEASE_FLAGS%
set LINKER_FLAGS=/INCREMENTAL:NO /opt:ref
set SYSTEM_LIBS=kernel32.lib user32.lib gdi32.lib winmm.lib ole32.lib d2d1.lib dxgi.lib d3d11.lib
set SRC=..\kimageprocessor.cpp ..\kd2dsurface.cpp ..\kimagingengine.cpp ..\kwindow.cpp
set BUILD_DIR=".\build"
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%
cl %COMPILER_FLAGS% %SRC% /link %LINKER_FLAGS% %SYSTEM_LIBS%
popd
echo Done
