@echo off
for /f "delims=" %%i in ('where cl') do set "CC=%%i"
for /f "delims=" %%i in ('where cl') do set "CXX=%%i"

set build_folder=release_build_cl
@echo on

rmdir /s /q %build_folder% 2>nul

mkdir %build_folder%
cd %build_folder%

cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .. && make
