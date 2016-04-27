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
filter { "configurations:Debug*" }
	defines { "_DEBUG" }
	optimize "Off"
	flags { "Symbols" }

filter { "configurations:DebugOpt*" }
	defines { "_DEBUG" }
	optimize "Debug"
	flags { "Symbols" }

filter { "configurations:Release*" }
	defines { "NDEBUG" }
	optimize "Full"
	flags { "NoFramePointer", "NoBufferSecurityCheck" }

filter { "configurations:Release*", "system:not Emscripten" }
	flags { "Symbols" }

-- platform config
filter { "system:windows" }
	defines { "WIN32", "_WINDOWS" }
	links { "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "comdlg32.lib", "advapi32.lib", "shell32.lib", "ole32.lib", "oleaut32.lib", "uuid.lib", "odbc32.lib", "odbccp32.lib" }

filter { "system:windows", "platforms:x86" }
	vectorextensions "SSE2"

filter { "system:linux" }
	links { "pthread" }
	links { "rt" }

filter { "configurations:NaCl64" }
	defines { "__native_client__" }
	targetsuffix "64"

filter { "configurations:NaCl32" }
	defines { "__native_client__" }
	targetsuffix "32"

filter { "configurations:NaClARM" }
	defines { "__native_client__" }
	targetsuffix "ARM"

filter { "configurations:PNaCl" }
	defines { "__native_client__" }

filter { "system:windows", "configurations:Release", "action:vs2012" }
	buildoptions { "/d2Zi+" }

filter { "system:windows", "configurations:Release", "action:vs2013" }
	buildoptions { "/Zo" }

filter { "system:windows", "configurations:Release" }
	flags { "NoIncrementalLink" }

filter { "system:windows", "platforms:x64", "configurations:Debug" }
	flags { "NoIncrementalLink" }

filter {}
