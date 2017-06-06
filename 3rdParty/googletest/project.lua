googletestPath = "googletest-1.7.0"

project(googletestPath)
	kind "StaticLib"
	language "C++"
	flags { "StaticRuntime", "OmitDefaultLibrary" }

	--  defines { "GTEST_HAS_PTHREAD=0" }

	includedirs { googletestPath }
	includedirs { googletestPath .. "/include" }

	files { googletestPath .. "/src/gtest-all.cc" }
	files { googletestPath .. "/include/gtest/*.h" }
	files { "project.lua" }

	-- include common stuff
	dofile "../../common-proj.lua"

	-- we use exceptions
	exceptionhandling "Default"

	warnings "Off"
