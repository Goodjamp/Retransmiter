rm RcModule.hex
rm TransmiterModule.hex

echo Build RcModule
cd RcModule

:: Use call command to provide retirn from the build.bat to this file. Originaly, without call,
:: we cant use nested *.bat file.
call build.bat RELEASE
cd ..

echo Build TransmiterModule
cd TransmiterModule
call build.bat RELEASE
cd ..

::copy result hex file to the root
copy RcModule\build\RcModule.hex RcModule.hex
copy TransmiterModule\build\TransmiterModule.hex TransmiterModule.hex