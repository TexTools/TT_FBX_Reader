# TexTools FBX Reader/Writer

A middleware executable for Reading and Writing FBX files to intermediate SQLite format for TexTools use.


# Build Instructions
In order to build the project, you'll need to get the following libraries, and post them in their respective folders in the /external/ folder at the root of the project.

- /external/fbx_sdk/ -> FBX SDK : https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-0
  - Set the install directory to the /external/ folder.
- /external/sqlite/ -> SQLite3 C++ Source Code : https://www.sqlite.org/download.html
  - Copy the raw .c and .h files in the zip to the /external/sqlite/ folder.
  
Furthermore, place whatever FBX you want to use as the test item when debugging at
- /sample/test.fbx
