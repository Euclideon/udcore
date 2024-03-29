project ("udCore" .. (projectSuffix or ""))
	kind "StaticLib"
	language "C++"
	staticruntime "On"
	flags { "OmitDefaultLibrary", "FatalWarnings" }

	files { "Source/**", "Include/**", "Docs/**" }

	files { "3rdParty/libdeflate/libdeflate.h", "3rdParty/libdeflate/lib/**" }
	includedirs { "3rdParty/libdeflate", "3rdParty/libdeflate/common" }

	files { "project.lua" }

	includedirs { "3rdParty" }
	includedirs { "3rdParty/mbedtls/include" }
	externalincludedirs { "3rdParty/mbedtls/include" }
	includedirs { "3rdParty/date/include" }
	externalincludedirs { "3rdParty/date/include" }
	includedirs { "Include" }

	-- include common stuff
	dofile "bin/premake-bin/common-proj.lua"

	files { "3rdParty/mbedtls/library/*.c", "3rdParty/mbedtls/include/mbedtls/*.h" }

	filter { "files:3rdParty/**" }
		warnings "Off"

	-- XCode4 can't set per-file configurations, so FatalWarnings needs to be off to build
	filter { "action:xcode4" }
		removeflags { "FatalWarnings" }

	-- VS2022 adds a define that is now a default define in the newer NDK, so FatalWarnings needs to be off to build
	filter { "action:vs2022", "system:android" }
		removeflags { "FatalWarnings" }

	filter { "configurations:Release", "system:Windows" }
		symbols "On"

	filter { "system:linux", "toolset:gcc" }
		buildoptions { "-pedantic" }
		enablewarnings { "undef" }

	filter { "system:macosx" }
		buildoptions { "-fno-stack-check" }

	filter { "configurations:Release", "system:macosx" }
		optimize "On"

	-- for windows, make the output name and location identical to that of udbin
	filter { "system:Windows" }
		targetdir "Lib/%{cfg.system}_%{cfg.shortname}"
		symbolspath "$(TargetDir)/$(ProjectName).pdb"

	filter { "system:uwp" }
		staticruntime "Off"
		exceptionhandling "On"

	filter {}
