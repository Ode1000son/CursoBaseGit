-- Premake5.lua simplificado para Aula 1.1
workspace "CursoOpenGL"
    configurations { "Debug", "Release" }
    architecture "x64"
    startproject "CursoOpenGL"
    location "build"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter {}

project "CursoOpenGL"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir "build/bin/%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.buildcfg}"

    files {
        "src/**.cpp",
        "vendor/glad/src/glad.c"
    }

    includedirs {
        "vendor/glad/include",
        "vendor/glfw-3.4/include"
    }

    libdirs {
        "vendor/glfw-3.4/build/src/%{cfg.buildcfg}"
    }

    links {
        "opengl32",
        "glfw3"
    }

    filter "configurations:Debug"
        postbuildcommands {
            "copy \"$(ProjectDir)..\\vendor\\glfw-3.4\\build\\src\\Debug\\glfw3.dll\" \"$(OutDir)\""
        }

    filter "configurations:Release"
        postbuildcommands {
            "copy \"$(ProjectDir)..\\vendor\\glfw-3.4\\build\\src\\Release\\glfw3.dll\" \"$(OutDir)\""
        }

    filter {}

