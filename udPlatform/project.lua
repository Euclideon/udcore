project "udPlatform"
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	files { "Source/**", "Include/**" }
	files { "project.lua" }

	includedirs { "../3rdParty" }
	includedirs { "../3rdParty/GL/freeglut/static/Include" }
	includedirs { "Include" }

	-- include common stuff
	dofile "../common-proj.lua"
