project ("udPlatform" .. (projectSuffix or ""))
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	files { "Source/**", "Include/**" }
	files { "project.lua" }

	includedirs { "../3rdParty" }
	includedirs { "Include" }

	configuration { "windows" }
		includedirs { "../3rdParty/sdl2/include" }

	configuration {}

	-- include common stuff
	dofile "../common-proj.lua"
