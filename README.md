# BatchCpp-for-MinGW

# **BatchCpp Generator**

A lightweight C++ utility that automatically analyzes your source code and generates a Windows Batch script (build_script.bat) to compile and link your project.

🚀 **Overview**
Manually typing long GCC commands with multiple include paths, library paths, and linker flags is tedious and error-prone.
BatchCpp automates this by scanning your project structure and identifying

Source files (.cpp, .cc, .cxx)
Library files (.lib, .a)
Required headers and associated libraries (e.g., SDL2, SFML, OpenGL, SQLite3)
Linker flags and preprocessor requirements (Unicode, GUI mode, PThreads)

✨ **Features**
Automatic Dependency Detection: Recognizes common libraries based on #include statements.
Smart Path Discovery: Automatically finds include directories and library paths by traversing your project folder.
Dual Build Modes: Generates both Debug (with -g3, -O0) and Release (with -O3, -march=native, -flto) configurations in a single script.
Smart Flags: Automatically detects if your project needs -pthread, -mwindows, or -municode based on your code content.

🛠 **How to Use**
Run the tool by passing your main source file as an argument:
cmd
BatchCpp.exe Main.cpp
Build your project: The tool will create a build_script.bat file in the same directory. Simply run it:
cmd
build_script.bat

📂 **Output & Files**
If the compilation is successful and no errors are reported, the following files will be generated in your project folder:
program_debug.exe: The debug version of your application (optimized for development).
program_release.exe: The release version of your application (optimized for performance).

🛠 **Requirements**
Windows OS
MinGW-w64 (GCC) installed and added to your PATH.

📜 License Free, Public Domain
