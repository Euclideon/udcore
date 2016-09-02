project ("udPlatform" .. (projectSuffix or ""))
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	files { "Source/**", "Include/**" }
	files { "project.lua" }

	includedirs { "../3rdParty" }
	includedirs { "Include" }

	-- include common stuff
	dofile "../common-proj.lua"
	
	filter { "configurations:Release", "system:Windows" }
		symbols "Off"
		
	filter {}
