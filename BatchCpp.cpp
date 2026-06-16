#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

struct ProjectData
{
    std::set<fs::path> sourceFiles;
    std::set<fs::path> includeDirs;
    std::set<fs::path> libraryPaths;

    std::vector<std::string> libraries;
    std::vector<std::string> linkerFlags;

    std::set<std::string> detectedIncludes;

    bool isWindowsGUI = false;
    bool isUnicode = false;
    bool needsPThread = false;
};

static const std::map<std::string, std::vector<std::string>> IncludeLibraries =
{
    {"SDL2/SDL.h", {"SDL2"}},
    {"SDL2/SDL_image.h", {"SDL2_image"}},
    {"SDL2/SDL_mixer.h", {"SDL2_mixer"}},
    {"SDL2/SDL_ttf.h", {"SDL2_ttf"}},
    {"SFML/Graphics.hpp", {"sfml-graphics","sfml-window","sfml-system"}},
    {"SFML/Audio.hpp", {"sfml-audio","sfml-system"}},
    {"SFML/Network.hpp", {"sfml-network","sfml-system"}},
    {"GLFW/glfw3.h", {"glfw3"}},
    {"GL/gl.h", {"opengl32"}},
    {"GL/glu.h", {"glu32"}},
    {"GL/glew.h", {"glew32"}},
    {"winsock2.h", {"ws2_32"}},
    {"ws2tcpip.h", {"ws2_32"}},
    {"mmsystem.h", {"winmm"}},
    {"curl/curl.h", {"curl"}},
    {"sqlite3.h", {"sqlite3"}},
    {"lua.hpp", {"lua"}},
    {"assimp/Importer.hpp", {"assimp"}},
    {"btBulletDynamicsCommon.h", {"BulletDynamics","BulletCollision","LinearMath"}},
    {"box2d/box2d.h", {"box2d"}},
    {"AL/al.h", {"openal32"}},
    {"portaudio.h", {"portaudio"}},
    {"FreeImage.h", {"FreeImage"}},
    {"UltraEngine.h", {"UltraEngine"}},
    {"Leadwerks.h", {"Leadwerks"}}
};

void addLibrary(ProjectData& data, const std::string& lib)
{
    if (std::find(data.libraries.begin(), data.libraries.end(), lib) == data.libraries.end())
        data.libraries.push_back(lib);
}

void analyzeFile(const fs::path& filePath, ProjectData& data)
{
    if (!fs::exists(filePath))
        return;

    if (data.sourceFiles.contains(filePath))
        return;

    data.sourceFiles.insert(filePath);

    std::ifstream file(filePath);
    if (!file.is_open())
        return;

// Ersetze die alten Regex-Definitionen durch diese:

    std::regex includeRegex(R"(^\s*#include\s*[<"]([^">]+)[">])"); // Diese kann meistens bleiben
    std::regex pragmaLibRegex("#pragma\\s+comment\\s*\\(\\s*lib\\s*,\\s*\"([^\"]+)\"\\s*\\)");
    std::regex pragmaLinkerRegex("#pragma\\s+comment\\s*\\(\\s*linker\\s*,\\s*\"([^\"]+)\"\\s*\\)");
    std::regex winMainRegex("\\bWinMain\\s*\\(");
    std::regex wmainRegex("\\bwmain\\s*\\(");




    std::string line;

    while (std::getline(file, line))
    {
        std::smatch match;

        if (std::regex_search(line, match, includeRegex))
        {
            std::string includeName = match[1];
            data.detectedIncludes.insert(includeName);

            auto libIt = IncludeLibraries.find(includeName);

            if (libIt != IncludeLibraries.end())
            {
                for (const auto& lib : libIt->second)
                    addLibrary(data, lib);
            }

            if (includeName == "thread" ||
                includeName == "mutex" ||
                includeName == "future" ||
                includeName == "condition_variable")
            {
                data.needsPThread = true;
            }

            fs::path localHeader = filePath.parent_path() / includeName;

            if (fs::exists(localHeader))
            {
                data.includeDirs.insert(localHeader.parent_path());
                analyzeFile(localHeader, data);
            }
        }

        if (std::regex_search(line, match, pragmaLibRegex))
        {
            std::string lib = match[1];

            if (lib.size() > 4 && lib.ends_with(".lib"))
                lib.erase(lib.size() - 4);

            if (lib.size() > 2 && lib.ends_with(".a"))
                lib.erase(lib.size() - 2);

            addLibrary(data, lib);
        }

        if (std::regex_search(line, match, pragmaLinkerRegex))
        {
            data.linkerFlags.push_back(match[1]);
        }

        if (std::regex_search(line, winMainRegex))
        {
            data.isWindowsGUI = true;
            data.isUnicode = true;
        }

        if (std::regex_search(line, wmainRegex))
        {
            data.isUnicode = true;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Verwendung: Generator <Hauptdatei.cpp>\n";
        return 1;
    }

    fs::path rootFile = fs::absolute(argv[1]);
    fs::path rootDir = rootFile.parent_path();

    ProjectData data;

    analyzeFile(rootFile, data);

    for (const auto& entry : fs::recursive_directory_iterator(rootDir))
    {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();

        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx")
            data.sourceFiles.insert(entry.path());

        if (ext == ".lib" || ext == ".a")
        {
            data.libraryPaths.insert(entry.path().parent_path());

            std::string name = entry.path().stem().string();

            if (name.rfind("lib", 0) == 0)
                name = name.substr(3);

            addLibrary(data, name);
        }
    }

    std::ofstream batch("build_script.bat");

    if (!batch.is_open())
    {
        std::cerr << "Konnte build_script.bat nicht erzeugen.\n";
        return 1;
    }

    batch << "@echo off\n";
    batch << "setlocal enabledelayedexpansion\n\n";

    auto writeBuild = [&](bool debug)
    {
        batch << "echo =====================================\n";

        if (debug)
            batch << "echo DEBUG BUILD\n";
        else
            batch << "echo RELEASE BUILD\n";

        if (debug)
            batch << "g++ -std=c++20 -g3 -O0 -Wall -Wextra -Wpedantic ";
        else
            batch << "g++ -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -s ";

        if (data.needsPThread)
            batch << "-pthread ";

        if (data.isWindowsGUI)
            batch << "-mwindows ";

        if (data.isUnicode)
            batch << "-municode ";

        for (const auto& dir : data.includeDirs)
            batch << "-I\"" << dir.string() << "\" ";

        for (const auto& libPath : data.libraryPaths)
            batch << "-L\"" << libPath.string() << "\" ";

        for (const auto& src : data.sourceFiles)
            batch << "\"" << src.string() << "\" ";

        for (const auto& flag : data.linkerFlags)
            batch << flag << " ";

        for (const auto& lib : data.libraries)
            batch << "-l" << lib << " ";

        batch << "-o ";
        batch << (debug ? "program_debug.exe" : "program_release.exe");
        batch << "\n\n";
    };

    writeBuild(true);
    writeBuild(false);

    batch.close();

    std::cout << "build_script.bat erfolgreich erstellt.\n";
    std::cout << "Quell-Dateien : " << data.sourceFiles.size() << "\n";
    std::cout << "Libraries     : " << data.libraries.size() << "\n";

    return 0;
}
