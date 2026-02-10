DEL /S *.sln
DEL /S *.vcxproj
DEL /S *.vcxproj.filters
DEL /S *.vcxproj.user
DEL /q .vs
rmdir /q /s build
rmdir /q /s Dependencies\Gemstone\build

CALL 3rdParty\premake\premake5.exe vs2022

pause