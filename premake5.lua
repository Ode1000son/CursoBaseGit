
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

    debugdir "$(ProjectDir).."

    files {
        "src/**.cpp",
        "vendor/glad/src/glad.c"
    }

    includedirs {
        "vendor/glad/include",
        "vendor/glfw-3.4/include",
        "vendor/glm",
        "vendor/stb",
        "vendor/assimp/include",
        "vendor/nlohmann/include",
        "vendor/miniaudio"
    }

    libdirs {
        "vendor/glfw-3.4/build/src/%{cfg.buildcfg}",
    }

    links {
        "opengl32",
        "glfw3",
        "vendor/assimp/assimp-vc143-mt.lib"
    }

    filter "configurations:Debug"
        postbuildcommands {
            "copy \"$(ProjectDir)..\\vendor\\glfw-3.4\\build\\src\\Debug\\glfw3.dll\" \"$(OutDir)\"",
            "copy \"$(ProjectDir)..\\vendor\\assimp\\assimp-vc143-mt.dll\" \"$(OutDir)\"",
            --copy assets folder to output directory
            "xcopy \"$(ProjectDir)..\\assets\" \"$(OutDir)assets\\\" /E /I /Y"
        }

    filter "configurations:Release"
        postbuildcommands {
            "copy \"$(ProjectDir)..\\vendor\\glfw-3.4\\build\\src\\Release\\glfw3.dll\" \"$(OutDir)\"",
            "copy \"$(ProjectDir)..\\vendor\\assimp\\assimp-vc143-mt.dll\" \"$(OutDir)\"",
            --copy assets folder to output directory
            "xcopy \"$(ProjectDir)..\\assets\" \"$(OutDir)assets\\\" /E /I /Y"
        }

    filter {}

