require "vstudio"

Root = "../"

function runtimeDependency(source, destination)
	postbuildcommands { ("{COPY} \"$(SolutionDir)Libraries/" .. source .. "\" \"$(OutDir)" .. destination .. "/\"") }
end

workspace "PSIA"
    location(Root)
    architecture "x64"
    startproject "Sender"
    language "C++"
	cppdialect "C++20"
    configurations { "Debug", "Release" }
    characterset "Unicode"
    system "windows"
    targetdir ("VS2022/%{prj.name}_%{cfg.buildcfg}")
    objdir ("VS2022/%{prj.name}.dir/")

    filter "configurations:Debug"
		runtime "Debug"
		defines { "PSIA_DEBUG" }
        symbols "On"
		optimize "Off"

	filter "configurations:Release"
 		runtime "Release"
		defines { "PSIA_RELEASE" }
        symbols "On"
		optimize "On"

	filter {}

    project "PSIA"
        location("%{wks.location}/%{prj.name}")
        -- pchheader "pch.h"
        -- pchsource("../Receiver/pch.cpp")
        kind "StaticLib"
        systemversion "latest"
        -- vectorextensions "AVX2"

        files {
            "%{wks.location}/%{prj.name}/**.cpp",
            "%{wks.location}/%{prj.name}/**.h",
        }

        includedirs {
            "$(SolutionDir)%{prj.name}"
        }

    project "Receiver"
        location("%{wks.location}/%{prj.name}")
        -- pchheader "pch.h"
        -- pchsource("../Receiver/pch.cpp")
        kind "ConsoleApp"
        systemversion "latest"
        -- vectorextensions "AVX2"

        files {
            "%{wks.location}/%{prj.name}/**.cpp",
            "%{wks.location}/%{prj.name}/**.h",
        }

        includedirs {
            "$(SolutionDir)%{prj.name}",
            "$(SolutionDir)PSIA/Source"
        }

        -- filter ("files:../Silver/External/**")
        --     flags { "NoPCH" }
        -- filter {}

        links {
            "PSIA",
            "ws2_32.lib"
        }

        -- libdirs {
        --     
        -- }

    project "Sender"
        location("%{wks.location}/%{prj.name}")
        -- pchheader "pch.h"
        -- pchsource("../Receiver/pch.cpp")
        kind "ConsoleApp"
        systemversion "latest"
        -- vectorextensions "AVX2"

        files {
            "%{wks.location}/%{prj.name}/**.cpp",
            "%{wks.location}/%{prj.name}/**.h",
        }

        includedirs {
            "$(SolutionDir)%{prj.name}",
            "$(SolutionDir)PSIA/Source"
        }

        -- filter ("files:../Silver/External/**")
        --     flags { "NoPCH" }
        -- filter {}

        links {
            "PSIA",
            "ws2_32.lib"
        }

        -- libdirs {
        --     
        -- }
