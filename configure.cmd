cmake -S . -B build -G "Unix Makefiles" || exit /b
copy build\compile_commands.json .\
