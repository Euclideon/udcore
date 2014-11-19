	project "udPlatform"
		language "C++"
		kind "StaticLib"
		flags { "StaticRuntime", "OmitDefaultLibrary" }

		-- setup paths --
		files { "Source/**.cpp", "Source/**.h", "Include/**.h" }
		files { "project.lua" }

--		includedirs { "Include/" }

		targetname "udPlatform"
		targetdir("lib")
		objdir "build"
