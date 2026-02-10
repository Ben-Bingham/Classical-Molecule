project "ImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

	flags "MultiProcessorCompile"

    filter "configurations:Debug"
        symbols "on"

    filter "configurations:Release"
        optimize "on"
    filter {}

	targetdir "%{wks.location}/build/3rdParty/bin/%{prj.name}"
	objdir "%{wks.location}/build/3rdParty/bin-int/%{prj.name}"

    files {
        "src/**.cpp",
        "src/**.h",
        "imgui.natvis",
        "imgui,natstepfilter"
    }

    links {
        "glfw3"
    }
    
    libdirs {
        "../GLFW/lib-vc2022"
    }

    includedirs {
        "../GLFW/include",
        "src"
    }    