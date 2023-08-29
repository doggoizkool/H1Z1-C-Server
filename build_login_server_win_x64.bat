@echo off

set OUTPUT_NAME=login_server_win_x64.exe
set SOURCES=..\src\win32_login_server.c

set DEFINES_SHARED=
set DEFINES=%DEFINES_SHARED%
set DEFINES_D=%DEFINES_SHARED% -DYOTE_INTERNAL

set LIBS_SHARED=-luser32 -lkernel32 -lws2_32 -lwinmm
set LIBS=%LIBS_SHARED% -lmsvcrt
set LIBS_D=%LIBS_SHARED% -lmsvcrt

set FLAGS_COMPILE_SHARED=-Wall -Wextra 
set FLAGS_COMPILE=%FLAGS_COMPILE_SHARED% -O2
set FLAGS_COMPILE_D=%FLAGS_COMPILE_SHARED% -O0 -g
rem set FLAGS_COMPILE_D=%FLAGS_COMPILE_SHARED% -O0 -g -fsanitize=address

set FLAGS_LINK_SHARED=
set FLAGS_LINK=%FLAGS_LINK_SHARED%
set FLAGS_LINK_D=%FLAGS_LINK_SHARED%

set COMPILE_ARGS=-o %OUTPUT_NAME% %DEFINES% %FLAGS_COMPILE% %SOURCES% %FLAGS_LINK% %LIBS%
set COMPILE_ARGS_D=-o %OUTPUT_NAME% %DEFINES_D% %FLAGS_COMPILE_D% %SOURCES% %FLAGS_LINK_D% %LIBS_D%

IF NOT EXIST build_login_server_win_x64 mkdir build_login_server_win_x64
pushd build_login_server_win_x64

gcc -o schema_tool.exe %FLAGS_COMPILE_D% -g -c "..\src\schema_tool.c"
.\schema_tool.exe ..\schema\login_udp_11.schm ..\schema\output\login_udp_11.c
gcc -o login_server_module.dll -O0 -g "..\src\h1z1_login_server.c" %FLAGS_COMPILE_D% %DEFINES_D% -shared %FLAGS_LINK_D% %LIBS_D%

echo LOCKED > .reload-lock
del .reload-lock

gcc -o login_server.exe -O0 -g "..\src\win32_login_server.c" %FLAGS_COMPILE_D% %DEFINES_D% %FLAGS_LINK_D% %LIBS_D%
popd
