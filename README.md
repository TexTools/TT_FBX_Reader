# TexTools FBX Converter
A middleware executable for Reading and Writing FBX files to intermediate SQLite format for TexTools use.


# Build Instructions
In order to build the project, you'll need to get the following libraries, and post them in their respective folders in the /external/ folder at the root of the project.

- /external/fbx_sdk/ -> FBX SDK : https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-0
  - Set the install directory to the /external/ folder.
- /external/sqlite/ -> SQLite3 C++ Source Code : https://www.sqlite.org/download.html
  - Copy the raw .c and .h files in the zip to the /external/sqlite/ folder.
- /external/eigen/ -> Eigen Math Library : http://eigen.tuxfamily.org/index.php?title=Main_Page
  - Copy the /eigen/ folder inside the zip ontop of your /eigen/ directory, so the raw files are in the same directory.
  
Furthermore, place whatever DB and FBX you want to use as the test items when debugging at
- /sample/test.fbx
- /sample/test.db

# Switching Modes
Simply providing a .fbx or .db file as the first argument to the executable will automatically identify the file extension and create a file of the other type at *result.fbx* or *result.db*.
For development purposes, you may need to change the Command Arguments under *Project Properties* -> *Debugging* to either the sample FBX or DB file as desired.
