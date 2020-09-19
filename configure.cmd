cmake -S . -B build -G "Unix Makefiles" -D USE_LUAJIT=ON || exit /b
copy build\compile_commands.json .\
