@echo off
echo Upgrading mupdf solution
devenv "%cd%\mupdf\platform\win32\mupdf.sln" /Upgrade

echo Building Release x64
msbuild "%cd%\mupdf\platform\win32\mupdf.vcxproj" /p:Configuration=Release /p:Platform=x64

echo Building Debug x64
msbuild "%cd%\mupdf\platform\win32\mupdf.vcxproj" /p:Configuration=Debug /p:Platform=x64

echo All done