workspace "WORKSPACE-NAME"
	configurations { "Debug", "Release" }
	platforms "x64"

	filter "platforms:x64"
		architecture "x64"
	filter "configurations:Debug"
		symbols "On"
	filter {}

	filter "configurations:Release"
		optimize "On"
	filter {}

	startproject "PROJECT-NAME"

	group "3rdParty"
		include "3rdParty/ImGui"
		include "3rdParty/glm"
		include "3rdParty/assimp"
		include "3rdParty/RenderingUtilities"
	group ""

project "PROJECT-NAME"
	kind "ConsoleApp"
	language "C++"

	cppdialect "C++20"

	flags "MultiProcessorCompile"

	targetdir ("%{wks.location}/build/bin/%{prj.name}")
	objdir ("%{wks.location}/build/bin-int/%{prj.name}")

	filter "configurations:Debug"
		symbols "On"
	filter {}
	
	filter "configurations:Release"
		optimize "On"
	filter {}

	files {
		"src/**.h",
		"src/**.cpp"
	}

	defines {
		"GLEW_STATIC"
	}

	includedirs {
		"src",
		"3rdParty/GLEW/include",
		"3rdParty/GLFW/include",
		"3rdParty/stb",
		"3rdParty/ImGui/src",
		"3rdParty/glm",
		"3rdParty/RenderingUtilities/src/",
		
		-- Assimp
		"3rdParty/assimp/ConfigurationHeaders/",
		"3rdParty/assimp/ConfigurationHeaders/assimp/",
		"3rdParty/assimp/include/"
	}

	libdirs {
        "3rdParty/GLFW/lib-vc2022",
        "3rdParty/GLEW/lib/Release/x64"
    }

	links {
        "glew32s",
        "opengl32",
        "glfw3",
		"ImGui",
		"glm",
		"assimp",
		"RenderingUtilities"
	}
