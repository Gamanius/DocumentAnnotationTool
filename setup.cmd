@echo off

IF EXIST "mupdf\platform\win32\x64" (
	echo Folder x64 exists... Skipping build
	EXIT /B 0
)

echo Upgrading mupdf solution
devenv "%cd%\mupdf\platform\win32\mupdf.sln" /Upgrade

echo Building Release x64
msbuild "%cd%\mupdf\platform\win32\mupdf.vcxproj" /p:Configuration=Release /p:Platform=x64

echo Building Debug x64
msbuild "%cd%\mupdf\platform\win32\mupdf.vcxproj" /p:Configuration=Debug /p:Platform=x64

echo All done