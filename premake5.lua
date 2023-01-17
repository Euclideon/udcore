require "vstudio"
require "emscripten"

newoption {
  trigger = "coverage",
  description = "Configures project to emit test coverage information"
}

filter { "options:coverage" }
	location "."
	buildoptions { "-fprofile-arcs", "-ftest-coverage" }
	linkoptions { "-fprofile-arcs" }
	optimize "Off"
filter {}

table.insert(premake.option.get("os").allowed, { "android", "Google Android" })
table.insert(premake.option.get("os").allowed, { "ios", "Apple iOS" })
table.insert(premake.option.get("os").allowed, { "emscripten", "Emscripten" })

-- iOS stuff
filter { "system:ios" }
	systemversion "10.3"
	links { "Foundation.framework" }
filter { "system:ios", "action:xcode4" }
	xcodebuildsettings {
		["CODE_SIGN_IDENTITY[sdk=iphoneos*]"] = "iPhone Developer",
		['IPHONEOS_DEPLOYMENT_TARGET'] = '10.3',
		['SDKROOT'] = 'iphoneos',
		['ARCHS'] = 'arm64',
		['TARGETED_DEVICE_FAMILY'] = "1,2",
		['DEVELOPMENT_TEAM'] = "452P989JPT",
		['ENABLE_BITCODE'] = "NO",
	}

-- Memory debug stuff
filter { "system:windows", "configurations:debug" }
	defines { "__MEMORY_DEBUG__" }

filter {}

solution "udCore"

	-- This hack just makes the VS project and also the makefile output their configurations in the idiomatic order
	if (_ACTION == "gmake" or _ACTION == "gmake2") and os.target() == premake.LINUX then
		configurations { "Release", "Debug", "DebugOpt", "ReleaseClang", "DebugClang", "DebugOptClang" }
		toolset "gcc"
		filter { "configurations:*Clang" }
			toolset "clang"
		filter { }
	elseif os.target() == premake.MACOSX or os.target() == premake.IOS or os.target() == premake.ANDROID then
		configurations { "Release", "Debug", "DebugOpt" }
		if os.target() == premake.MACOSX or os.target() == premake.IOS then
			toolset "clang"
		end
	else
		configurations { "Debug", "DebugOpt", "Release" }
	end

	if os.target() == premake.IOS or os.target() == premake.ANDROID then
		platforms { "x64", "arm64" }
	elseif os.target() == "emscripten" then
		platforms { "Emscripten" }
		buildoptions { "-s USE_PTHREADS=1" }
		linkoptions  { "-s USE_PTHREADS=1", "-s PTHREAD_POOL_SIZE=8", "-s ABORTING_MALLOC=0", "-s FETCH=1", "-s PROXY_TO_PTHREAD=1", "-s WASM=1", "-s TOTAL_MEMORY=1073741824" }
		targetextension ".bc"
		filter { "kind:*App" }
			targetextension ".js"
		filter {}
	else
		platforms { "x64" }
	end

	pic "On"
	editorintegration "on"
	cppdialect "C++20"
	filter { "system:android", "files:**.cpp" }
		buildoptions { "-std=c++20" }
	filter {}
	if os.target() == premake.LINUX then
		-- Ubuntu 20.04 only supports C++2a
		local pipe = io.popen("gcc -std=c++20 2>&1")
		local result = pipe:read('*a')
		if string.find(result, "did you mean ‘-std=c++2a’?", 0, true) then
			cppdialect "C++2a"
		end
	end

	startproject "udTest"

	-- CI defines
	if os.getenv("CI_COMMIT_REF_NAME") then
		defines { "GIT_BRANCH=\"" .. os.getenv("CI_COMMIT_REF_NAME") .. "\"" }
	end
	if os.getenv("CI_COMMIT_SHA") then
		defines { "GIT_REF=\"" .. os.getenv("CI_COMMIT_SHA") .. "\"" }
	end
	if os.getenv("CI_PIPELINE_ID") then
		defines { "GIT_BUILD=" .. os.getenv("CI_PIPELINE_ID") }
	end

	group "libs"
	if os.target() ~= premake.IOS and os.target() ~= premake.ANDROID then
		dofile "3rdParty/googletest/project.lua"
	end

	group ""

	dofile "project.lua"

	if os.target() ~= premake.IOS and os.target() ~= premake.ANDROID then
		dofile "udTest/project.lua"
	end
