project "udKernel"
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	files { "Source/**", "Include/**" }
	files { "project.lua" }

	includedirs { "Include" }
	includedirs { "../udPlatform/Include" }

	configuration { "PNaCl" }
		buildoptions { "-std=gnu++11" }

	-- include common stuff
	dofile "../common-proj.lua"
