git clean -f -d

if [ $# -eq 0 ]; then
	./build.sh Debug x64;
	./build.sh Release x64;
else
	git submodule sync
	if [ $? -ne 0 ]; then exit 3; fi
	git submodule update --init
	if [ $? -ne 0 ]; then exit 3; fi
	git submodule foreach --recursive "git submodule sync && git submodule update --init"
	if [ $? -ne 0 ]; then exit 3; fi

	if [ $OSTYPE == "msys" ]; then # Windows, MingW
		if [ $# -gt 2 ]; then
			bin/premake-bin/premake5.exe vs2019 --os=$3
			if [ $? -ne 0 ]; then exit 4; fi
		else
			bin/premake-bin/premake5.exe vs2019
			if [ $? -ne 0 ]; then exit 4; fi
		fi

		"C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/MSBuild/Current/Bin/amd64/MSBuild.exe" udCore.sln //p:Configuration=$1 //p:Platform=$2 //v:m //m
		if [ $? -ne 0 ]; then exit 5; fi

		if [ $# -eq 2 ]; then
			Output/bin/${1}_${2}/udTest.exe
			if [ $? -ne 0 ]; then exit 1; fi
		fi
	elif ([[ $OSTYPE == "darwin"* ]] && [ $# -ge 3 ] && [ $3 == "xcode" ]); then # macOS XCode build (only used to ensure project builds in xcode)
		bin/premake-bin/premake5-osx xcode4 $PREMAKEOPTIONS
		if [ $? -ne 0 ]; then exit 4; fi

		xcodebuild -project udTest/udTest.xcodeproj -configuration $1 -arch $2
		if [ $? -ne 0 ]; then exit 5; fi

		Output/bin/${1}_x64/udTest
		if [ $? -ne 0 ]; then exit 1; fi
	elif ([[ $OSTYPE == "darwin"* ]] && [ $# -ge 3 ] && [ $3 == "ios" ]); then # iOS XCode build (only used to ensure project builds in xcode)
		bin/premake-bin/premake5-osx xcode4 --os=ios
		if [ $? -ne 0 ]; then exit 4; fi

		if [ $2 == "x86_64" ]; then # Simulator
			xcodebuild -project udCore.xcodeproj -configuration $1 -arch $2 -sdk iphonesimulator
			if [ $? -ne 0 ]; then exit 5; fi
		else
			xcodebuild -project udCore.xcodeproj -configuration $1 -arch $2
			if [ $? -ne 0 ]; then exit 5; fi
		fi
	elif ([ $2 == "Emscripten" ]); then # Emscripten
		pushd /emsdk
		source ./emsdk_env.sh
		popd
	
		bin/premake-bin/premake5 gmake2 --os=emscripten
		if [ $? -ne 0 ]; then exit 4; fi

		make config=$(echo ${1}_${2} | tr [:upper:] [:lower:]) -j4
		if [ $? -ne 0 ]; then exit 5; fi

		# TODO: Figure out how to run unit tests in a headless browser
	else
		if ([ $# -ge 3 ] && [ $3 == "Coverage" ]); then
			export PREMAKEOPTIONS="--coverage"
		fi

		# Allow core dumps to be saved for non-packaged binaries
		mkdir -p /tmp/cores

		bin/premake-bin/premake5 gmake2 $PREMAKEOPTIONS
		if [ $? -ne 0 ]; then exit 4; fi

		make config=$(echo ${1}_${2} | tr [:upper:] [:lower:]) -j4
		if [ $? -ne 0 ]; then exit 5; fi

		if ([ $# -ge 3 ] && [ $3 == "memcheck" ]); then
			valgrind --tool=memcheck --error-exitcode=1 Output/bin/${1}_${2}/udTest
		elif ([ $# -ge 3 ] && [ $3 == "helgrind" ]); then
			# We should look into using `--free-is-write=yes`
			valgrind --tool=helgrind --error-exitcode=1 Output/bin/${1}_${2}/udTest
		else
			Output/bin/${1}_${2}/udTest
		fi

		if [ $# -ge 3 ] && [ $3 == "Coverage" ]; then
			gcovr -r . -e 3rdParty -e udTest -s -p
			exit 0;
		fi

		if [ $# -ge 3 ] && [ $3 == "valgrind" ]; then
			exit 0;
		fi
	fi
fi
