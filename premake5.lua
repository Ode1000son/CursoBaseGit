
local project_root = os.getcwd()
local vendor_root = path.join(project_root, "vendor")
local assets_dir = path.join(project_root, "assets")

local function winpath(p)
    return string.gsub(p, "/", "\\")
end

local physx_root = path.join(vendor_root, "physx/install/vc17win64-cpu-only/PhysX")
local physx_include_dir = path.join(physx_root, "include")
local physx_bin_root = path.join(physx_root, "bin/win.x86_64.vc143.mt")
local physx_debug_bin = path.join(physx_bin_root, "debug")
local physx_release_bin = path.join(physx_bin_root, "release")
local physx_dlls = {
    "PhysX_64.dll",
    "PhysXCommon_64.dll",
    "PhysXFoundation_64.dll",
    "PhysXCooking_64.dll",
    "PVDRuntime_64.dll"
}

local function copy_physx_dlls(bin_dir)
    local commands = {}
    for _, dll in ipairs(physx_dlls) do
        table.insert(commands, "copy \"" .. winpath(path.join(bin_dir, dll)) .. "\" \"$(OutDir)\"")
    end
    return commands
end

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
    staticruntime "On"

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
        "vendor/miniaudio",
        physx_include_dir
    }

    links {
        "opengl32",
        "ole32",
        "ws2_32",
        "Winmm",
        "Advapi32",
        "glfw3",
        path.join(vendor_root, "assimp/assimp-vc143-mt.lib"),
        "PhysX_64",
        "PhysXCommon_64",
        "PhysXFoundation_64",
        "PhysXExtensions_static_64",
        "PhysXPvdSDK_static_64",
        "PhysXTask_static_64",
        "PhysXCooking_64",
        "PhysXCharacterKinematic_static_64",
        "PhysXVehicle2_static_64",
        "PVDRuntime_64"
    }

    filter "configurations:Debug"
        libdirs {
            path.join(vendor_root, "glfw-3.4/build/src/Debug"),
            physx_debug_bin
        }
        postbuildcommands(table.join({
            "copy \"" .. winpath(path.join(vendor_root, "glfw-3.4/build/src/Debug/glfw3.dll")) .. "\" \"$(OutDir)\"",
            "copy \"" .. winpath(path.join(vendor_root, "assimp/assimp-vc143-mt.dll")) .. "\" \"$(OutDir)\"",
            "xcopy \"" .. winpath(assets_dir) .. "\" \"$(OutDir)assets\\\" /E /I /Y"
        }, copy_physx_dlls(physx_debug_bin)))

    filter "configurations:Release"
        libdirs {
            path.join(vendor_root, "glfw-3.4/build/src/Release"),
            physx_release_bin
        }
        postbuildcommands(table.join({
            "copy \"" .. winpath(path.join(vendor_root, "glfw-3.4/build/src/Release/glfw3.dll")) .. "\" \"$(OutDir)\"",
            "copy \"" .. winpath(path.join(vendor_root, "assimp/assimp-vc143-mt.dll")) .. "\" \"$(OutDir)\"",
            "xcopy \"" .. winpath(assets_dir) .. "\" \"$(OutDir)assets\\\" /E /I /Y"
        }, copy_physx_dlls(physx_release_bin)))

    filter {}

