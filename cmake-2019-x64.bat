set GENERATE_OPTIONS= -G "Visual Studio 16 2019" -A x64
set GENERATE_PROJECT_NAME=.build-2019-x64

cd %~dp0
mkdir %GENERATE_PROJECT_NAME%
cd %GENERATE_PROJECT_NAME%
call cmake %GENERATE_OPTIONS% "%~dp0"
cd %~dp0
