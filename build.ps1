#!/usr/bin/env powershell

#Remove-Item build -Force -Recurse -ErrorAction SilentlyContinue
New-Item -Path .\build_win -ItemType directory -Force
Set-Location build_win

$TC_FILE = "-DCMAKE_TOOLCHAIN_FILE=$env:WORKSPACE\vcpkg\scripts\buildsystems\vcpkg.cmake"
$VCPKG_TRIPLET = "-DVCPKG_TARGET_TRIPLET=$env:VCPKG_DEFAULT_TRIPLET"
if ( $args[0] -eq "VS" )
{
  # VS
  cmake -G "Visual Studio 15 Win64" "-DCMAKE_BUILD_TYPE=Release" $TC_FILE $VCPKG_TRIPLET ..
  cmake --build . --config Release --target install
}
else
{
  # Ninja
  cmake -G "Ninja" "-DCMAKE_BUILD_TYPE=Release" $TC_FILE $VCPKG_TRIPLET ..
  cmake --build . --config Release --target install
}

Set-Location ..