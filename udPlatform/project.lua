project ("udPlatform" .. (projectSuffix or ""))
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	files { "Source/**", "Include/**", "Docs/**" }
	files { "../3rdParty/mbedtls/config.h" }
	files { "../3rdParty/mbedtls/library/aes.c", "../3rdParty/mbedtls/include/mbedtls/aes.h" }
	files { "../3rdParty/mbedtls/library/sha1.c", "../3rdParty/mbedtls/include/mbedtls/sha1.h" }
	files { "../3rdParty/mbedtls/library/sha256.c", "../3rdParty/mbedtls/include/mbedtls/sha256.h" }
	files { "../3rdParty/mbedtls/library/sha512.c", "../3rdParty/mbedtls/include/mbedtls/sha512.h" }
	files { "project.lua" }

	includedirs { "../3rdParty" }
	includedirs { "../3rdParty/mbedtls/include" }
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
