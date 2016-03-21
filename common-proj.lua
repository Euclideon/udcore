-- clear global state
filter {}
configuration {}

-- common stuff that we want to be common among all projects
warnings "Extra"

targetname "%{prj.name}"

flags { "NoMinimalRebuild", "NoPCH" }
exceptionhandling "Off"
rtti "Off"
floatingpoint "Fast"

-- TODO: the original project files had some options that premake can't express, we should add them to premake?
--  * NoFloatingPointExceptions
--  * Function Level Linking
--  * Enable string pooling
--  * Inline function expansion
--  * Enable intrinsic functions
--  * Randomize base address
--  * Profile


objdir "Output/intermediate/%{prj.name}/%{cfg.buildcfg}_%{cfg.platform}"
filter { "kind:SharedLib or StaticLib" }
	targetdir "Output/lib/%{cfg.buildcfg}_%{cfg.platform}"
filter { "kind:ConsoleApp or WindowedApp" }
	targetdir "Output/bin/%{cfg.buildcfg}_%{cfg.platform}"

-- configurations
configuration { "Debug*" }
	defines { "_DEBUG" }
	optimize "Off"
	flags { "Symbols" }

configuration { "DebugOpt*" }
	defines { "_DEBUG" }
	optimize "Debug"
	flags { "Symbols" }

configuration { "Release*" }
	defines { "NDEBUG" }
	optimize "Full"
	flags { "NoFramePointer", "NoBufferSecurityCheck" }
	
configuration { "Release*", "not Emscripten" }
	flags { "Symbols" }

-- platform config
configuration { "windows" }
	defines { "WIN32", "_WINDOWS" }
	links { "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "comdlg32.lib", "advapi32.lib", "shell32.lib", "ole32.lib", "oleaut32.lib", "uuid.lib", "odbc32.lib", "odbccp32.lib" }

configuration { "windows", "x86" }
	vectorextensions "SSE2"

configuration { "linux" }
	links { "pthread" }
	links { "rt" }

configuration { "NaCl64" }
	defines { "__native_client__" }
	targetsuffix "64"

configuration { "NaCl32" }
	defines { "__native_client__" }
	targetsuffix "32"

configuration { "NaClARM" }
	defines { "__native_client__" }
	targetsuffix "ARM"

configuration { "PNaCl" }
	defines { "__native_client__" }

configuration { "windows", "Release", "vs2012" }
	buildoptions { "/d2Zi+" }

configuration { "windows", "Release", "vs2013" }
	buildoptions { "/Zo" }

configuration { "windows", "Release" }
	flags { "NoIncrementalLink" }

configuration { "windows", "x64", "Debug" }
	flags { "NoIncrementalLink" }

configuration {}
