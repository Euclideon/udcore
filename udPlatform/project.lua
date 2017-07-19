project ("udPlatform" .. (projectSuffix or ""))
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	files { "Source/**", "Include/**", "Docs/**" }
	files { "project.lua" }

	includedirs { "../3rdParty" }
	includedirs { "Include" }

	-- include common stuff
	dofile "../common-proj.lua"

	filter { "configurations:Release", "system:Windows" }
		symbols "On"

	-- for windows, make the output name and location identical to that of udbin
	filter { "system:Windows" }
		targetdir "../Lib/%{cfg.system}_%{cfg.shortname}"
		symbolspath "$(TargetDir)/$(ProjectName).pdb"

	-- for nacl, make the output name and location identical to that of udbin
	filter { "system:nacl" }
		targetdir "../Lib/windows_%{cfg.shortname}"

	filter {}
