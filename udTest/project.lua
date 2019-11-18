project ("udTest" .. (projectSuffix or ""))
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++14"
	staticruntime "On"

	includedirs { "../3rdParty" }
	includedirs { "../3rdParty/googletest/include" }

	includedirs { "../Include" }

	files { "src/**.cpp", "src/**.h" }
	files { "project.lua" }

	links { "udCore" .. (projectSuffix or "") }
	links { "googletest" }

	filter { }

	defines { "_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING" }

	filter { "system:windows" }
		links { "winmm.lib" }

	filter { "system:linux" }
		links { "z", "dl" }

	filter { "system:macosx" }
		links { "Foundation.framework", "Security.framework" }
		buildoptions { "-fno-stack-check" }

	-- include common stuff
	dofile "../bin/premake-bin/common-proj.lua"
	exceptionhandling "Default"

	debugdir "../"
