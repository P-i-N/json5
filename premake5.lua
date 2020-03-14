workspace "json5"
	-- Premake output folder
	location(path.join(".build", _ACTION))

	-- Target architecture
	architecture "x86_64"

	-- Configuration settings
	configurations { "Debug", "Release" }

	-- Debug configuration
	filter { "configurations:Debug" }
		defines { "DEBUG" }
		symbols "On"
		optimize "Off"

	-- Release configuration
	filter { "configurations:Release" }
		defines { "NDEBUG" }
		optimize "Speed"
		inlining "Auto"

	filter { "language:not C#" }
		defines { "_CRT_SECURE_NO_WARNINGS" }
		characterset ("MBCS")
		buildoptions { "/std:c++latest" }

	filter { }
		targetdir ".bin/%{cfg.longname}/"
		defines { "WIN32", "_AMD64_" }
		--exceptionhandling "Off"
		rtti "Off"
		vectorextensions "AVX2"

-------------------------------------------------------------------------------

project "test"
	language "C++"
	kind "ConsoleApp"
	files { "test/**.cpp", "test/**.hpp", "include/**.hpp" }
	includedirs { "include" }
	debugdir "test"
