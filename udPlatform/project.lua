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
	files { "../3rdParty/mbedtls/library/rsa.c", "../3rdParty/mbedtls/include/mbedtls/rsa.h" }
	files { "../3rdParty/mbedtls/library/bignum.c", "../3rdParty/mbedtls/include/mbedtls/bignum.h" }
	files { "../3rdParty/mbedtls/library/entropy.c", "../3rdParty/mbedtls/include/mbedtls/entropy.h" }
	files { "../3rdParty/mbedtls/library/entropy_poll.c", "../3rdParty/mbedtls/include/mbedtls/entropy_poll.h" }
	files { "../3rdParty/mbedtls/library/ctr_drbg.c", "../3rdParty/mbedtls/include/mbedtls/ctr_drbg.h" }
	files { "../3rdParty/mbedtls/library/md.c", "../3rdParty/mbedtls/include/mbedtls/md.h" }
	files { "../3rdParty/mbedtls/library/md_wrap.c", "../3rdParty/mbedtls/include/mbedtls/md_wrap.h" }
	files { "../3rdParty/mbedtls/library/asn1parse.c", "../3rdParty/mbedtls/include/mbedtls/asn1parse.h" }
	files { "../3rdParty/mbedtls/library/oid.c", "../3rdParty/mbedtls/include/mbedtls/oid.h" }
	files { "../3rdParty/mbedtls/library/ripemd160.c", "../3rdParty/mbedtls/include/mbedtls/ripemd160.h" }
	files { "../3rdParty/mbedtls/library/md5.c", "../3rdParty/mbedtls/include/mbedtls/md5.h" }
	files { "../3rdParty/mbedtls/library/timing.c", "../3rdParty/mbedtls/include/mbedtls/timing.h" }

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
