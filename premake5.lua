require "ios"
require "android"
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
filter {}

-- Android stuff (This should be handled much better)
filter { "system:android" }
  toolchainversion "4.9"
  systemversion "21"
  defines { "_LARGEFILE_SOURCE" }
  buildoptions {
    "-ffast-math",
    "-pthread",
    "-fexcess-precision=fast"
  }
filter {}

-- TODO: Remove this when moving to a more updated version than alpha12
premake.override(premake.vstudio.vc2010.elements, "clCompile", function(oldfn, cfg)
	local calls = oldfn(cfg)

	-- Inject programDatabaseFileName support back in
	if cfg.kind == premake.STATICLIB then
		table.insert(calls, function (cfg)
			if cfg.symbolspath and cfg.symbols ~= premake.OFF and cfg.debugformat ~= "c7" then
				premake.vstudio.vc2010.element("ProgramDataBaseFileName", nil, premake.project.getrelative(cfg.project, cfg.symbolspath))
			end
		end)
	end

	return calls
end)

-- TODO: Move this into the emscripten module
function premake.modules.emscripten.emcc.getrunpathdirs(cfg, dirs)
	return premake.tools.clang.getrunpathdirs(cfg, dirs)
end

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
		buildoptions { "-s USE_PTHREADS=1", "-s PTHREAD_POOL_SIZE=8", "-s ABORTING_MALLOC=0", "-s FETCH=1", "-s PROXY_TO_PTHREAD=1", "-s WASM=0", "-s TOTAL_MEMORY=1073741824" }
		linkoptions  { "-s USE_PTHREADS=1", "-s PTHREAD_POOL_SIZE=8", "-s ABORTING_MALLOC=0", "-s FETCH=1", "-s PROXY_TO_PTHREAD=1", "-s WASM=0", "-s TOTAL_MEMORY=1073741824" }
		targetextension ".bc"
		filter { "kind:*App" }
			targetextension ".js"
		filter { "files:**.cpp" }
			buildoptions { "-std=c++11" }
		filter {}
	else
		platforms { "x64" }
	end

	pic "On"
	editorintegration "on"
	cppdialect "C++11"
	xcodebuildsettings { ["CLANG_CXX_LANGUAGE_STANDARD"] = "c++0x" } -- XCode doesn't support c++11, needs to be c++0x
	editandcontinue "Off"

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
