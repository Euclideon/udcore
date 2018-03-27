newoption {
	trigger = "mbedTLS",
	value = "VALUE",
	description = "Specify whether you want full or minimal mbedTLS support",
	allowed = {
		{ "full", "Full mbedTLS support" },
		{ "minimal", "Minimal mbedTLS support" },
	},
	default = "minimal",
}

project ("udPlatform" .. (projectSuffix or ""))
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	files { "Source/**", "Include/**", "Docs/**" }

	files { "project.lua" }

	includedirs { "../3rdParty" }
	includedirs { "../3rdParty/mbedtls/include" }
	includedirs { "Include" }

	-- include common stuff
	dofile "../common-proj.lua"

	filter { "options:mbedTLS=minimal" }
		files { "../3rdParty/mbedtls/include/mbedtls/config.h" }
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
		files { "../3rdParty/mbedtls/library/asn1write.c", "../3rdParty/mbedtls/include/mbedtls/asn1write.h" }
		files { "../3rdParty/mbedtls/library/oid.c", "../3rdParty/mbedtls/include/mbedtls/oid.h" }
		files { "../3rdParty/mbedtls/library/timing.c", "../3rdParty/mbedtls/include/mbedtls/timing.h" }
		files { "../3rdParty/mbedtls/library/dhm.c", "../3rdParty/mbedtls/include/mbedtls/dhm.h" }
		files { "../3rdParty/mbedtls/library/ecp.c", "../3rdParty/mbedtls/include/mbedtls/ecp.h" }
		files { "../3rdParty/mbedtls/library/ecp_curves.c", "../3rdParty/mbedtls/include/mbedtls/ecp_curves.h" }
		files { "../3rdParty/mbedtls/library/ecdsa.c", "../3rdParty/mbedtls/include/mbedtls/ecdsa.h" }
		files { "../3rdParty/mbedtls/library/hmac_drbg.c", "../3rdParty/mbedtls/include/mbedtls/hmac_drbg.h" }

	filter { "options:mbedTLS=full" }
		files { "../3rdParty/mbedtls/library/*.c", "../3rdParty/mbedtls/include/mbedtls/*.h" }
		defines { "FULL_MBEDTLS" }

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
