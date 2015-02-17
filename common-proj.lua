
-- common stuff that we want to be common among all projects
warnings "Extra"

targetname "%{prj.name}"
targetdir "Output/Lib"
objdir "Output/Intermediate/%{prj.name}"

-- configurations
configuration { "Debug" }
	defines { "_DEBUG" }
	optimize "Debug"
	flags { "Symbols" }

configuration { "DebugOpt" }
	defines { "_DEBUG" }
	optimize "On"
	flags { "Symbols" }

configuration { "Release" }
	defines { "NDEBUG" }
	optimize "Full"

-- platform config
configuration { "windows" }
	defines { "WIN32", "_WINDOWS" }
	links { "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "comdlg32.lib", "advapi32.lib", "shell32.lib", "ole32.lib", "oleaut32.lib", "uuid.lib", "odbc32.lib", "odbccp32.lib" }

configuration { "linux" }
--	links { "gl" }

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

configuration {}
