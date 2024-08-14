rem "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cl /EHsc widecar.cpp /link User32.lib /SUBSYSTEM:WINDOWS
mt -manifest widecar.manifest -outputresource:widecar.exe;#1
