cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug || exit /b
copy build\compile_commands.json . || exit /b
