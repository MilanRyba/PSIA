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
    configurations { "Debug", "Release", "Net Derper"}
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

    filter "configurations:Net Derper"
 	    runtime "Debug"
	    defines { "PSIA_NET_DERPER" }
        symbols "On"
	    optimize "Off"

	filter {}

    project "PSIA"
        location("%{wks.location}/%{prj.name}")
        -- pchheader "pch.h"
        -- pchsource("../Receiver/pch.cpp")
        kind "StaticLib"
        systemversion "latest"

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

        files {
            "%{wks.location}/%{prj.name}/**.cpp",
            "%{wks.location}/%{prj.name}/**.h",
        }

        removefiles { 
            "SelectiveRepeatReceiver.cpp",
        }

        includedirs {
            "$(SolutionDir)%{prj.name}",
            "$(SolutionDir)PSIA/Source"
        }

        links {
            "PSIA",
            "ws2_32.lib"
        }

    project "Sender"
        location("%{wks.location}/%{prj.name}")
        -- pchheader "pch.h"
        -- pchsource("../Receiver/pch.cpp")
        kind "ConsoleApp"
        systemversion "latest"

        files {
            "%{wks.location}/%{prj.name}/**.cpp",
            "%{wks.location}/%{prj.name}/**.h",
        }

        removefiles { 
            "SelectiveRepeatSender.cpp",
        }

        includedirs {
            "$(SolutionDir)%{prj.name}",
            "$(SolutionDir)PSIA/Source"
        }

        links {
            "PSIA",
            "ws2_32.lib"
        }
