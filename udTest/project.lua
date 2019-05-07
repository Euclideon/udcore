project ("udTest" .. (projectSuffix or ""))
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++11"
	flags { "StaticRuntime" }

	includedirs { "../3rdParty" }
	includedirs { "../3rdParty/googletest/" .. googletestPath .. "/include" }

	includedirs { "../Include" }

	files { "src/**.cpp", "src/**.h" }
	files { "project.lua" }

	links { "udCore" .. (projectSuffix or "") }
	links { googletestPath }

	filter { }

	defines { "_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING" }

	filter { "system:windows" }
		links { "winmm.lib" }

	filter { "system:linux" }
		links { "z", "dl" }

	filter { "system:macosx" }
		links { "Foundation.framework", "Security.framework" }

	-- include common stuff
	dofile "../common-proj.lua"
	exceptionhandling "Default"

	debugdir "../"
