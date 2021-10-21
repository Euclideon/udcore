project "googletest"
	kind "StaticLib"
	language "C++"
	staticruntime "On"
	flags { "OmitDefaultLibrary" }

	--  defines { "GTEST_HAS_PTHREAD=0" }
	includedirs { "." }
	includedirs { "include" }

	files { "src/gtest-all.cc" }
	files { "include/gtest/*.h" }
	files { "project.lua" }

	filter { "system:uwp" }
		staticruntime "Off"
	filter {}

	-- include common stuff
	dofile "../../bin/premake-bin/common-proj.lua"

	-- we use exceptions
	exceptionhandling "Default"

	warnings "Off"
