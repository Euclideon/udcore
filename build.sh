git clean -f -d

if [ $# -eq 0 ]; then
	./build.sh Debug x64;
	./build.sh Release x64;
else
	git submodule sync --recursive
	git submodule update --init --recursive
	if [ $? -ne 0 ]; then exit 3; fi

	if [ $OSTYPE == "msys" ]; then # Windows, MingW
		bin/premake-bin/premake5.exe vs2015
		if [ $? -ne 0 ]; then exit 4; fi

		"C:/Program Files (x86)/MSBuild/14.0/Bin/amd64/MSBuild.exe" udCore.sln //p:Configuration=$1 //p:Platform=$2 //v:m //m
		if [ $? -ne 0 ]; then exit 5; fi

		Output/bin/${1}_${2}/udTest.exe
		if [ $? -ne 0 ]; then exit 1; fi
	elif ([[ $OSTYPE == "darwin"* ]] && [ $# -ge 3 ] && [ $3 == "xcode" ]); then # macOS XCode build (only used to ensure project builds in xcode)
		bin/premake-bin/premake5-osx xcode4 $PREMAKEOPTIONS
		if [ $? -ne 0 ]; then exit 4; fi

		xcodebuild -project udTest/udTest.xcodeproj -configuration $1 -arch $2
		if [ $? -ne 0 ]; then exit 5; fi

		Output/bin/${1}_x64/udTest
		if [ $? -ne 0 ]; then exit 1; fi
	elif ([[ $OSTYPE == "darwin"* ]] && [ $# -ge 4 ] && [ $3 == "ios" ] && [ $4 == "xcode" ]); then # iOS XCode build (only used to ensure project builds in xcode)
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
		if [ $# -ge 3 ]; then
			if [ $3 == "ios" ]; then
				export PREMAKEOPTIONS="--os=ios"
			elif [ $3 == "android" ]; then
				export NDKROOT="/android-ndk-r13b"
				export PREMAKEOPTIONS="--os=android"
			elif [ $3 == "Coverage" ]; then
				export PREMAKEOPTIONS="--coverage"
			fi
		fi

		if [[ $OSTYPE == "darwin"* ]]; then # OSX, Sierra
			bin/premake-bin/premake5-osx gmake2 $PREMAKEOPTIONS
		else
			# Allow core dumps to be saved for non-packaged binaries
			mkdir -p /tmp/cores

			bin/premake-bin/premake5 gmake2 $PREMAKEOPTIONS
		fi
		if [ $? -ne 0 ]; then exit 4; fi

		make config=$(echo ${1}_${2} | tr [:upper:] [:lower:]) -j4
		if [ $? -ne 0 ]; then exit 5; fi

		if [ $# -eq 2 ] || ([ $3 != "ios" ] && [ $3 != "android" ]); then
			if [ $3 == "memcheck" ]; then
				valgrind --tool=memcheck --error-exitcode=1 Output/bin/${1}_${2}/udTest
			elif [ $3 == "helgrind" ]; then
				# We should look into using `--free-is-write=yes`
				valgrind --tool=helgrind --error-exitcode=1 Output/bin/${1}_${2}/udTest
			else
				Output/bin/${1}_${2}/udTest
			fi
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
