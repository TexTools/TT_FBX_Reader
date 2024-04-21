# TexTools FBX Converter
A middleware executable for Reading and Writing FBX files to intermediate SQLite format for TexTools use.


# Build Instructions
In order to build the project, you'll need to get the following libraries, and post them in their respective folders in the /external/ folder at the root of the project.

- /external/fbx_sdk/ -> FBX SDK : https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-0
  - Set the install directory to the /external/ folder.
- /external/sqlite/ -> SQLite3 C++ Source Code : https://www.sqlite.org/download.html
  - Copy the raw .c and .h files in the zip to the /external/sqlite/ folder.
  
Furthermore, place whatever DB and FBX you want to use as the test items when debugging at
- /sample/test.fbx
- /sample/test.db

# Switching Modes
Simply providing a .fbx or .db file as the first argument to the executable will automatically identify the file extension and create a file of the other type at *result.fbx* or *result.db*.

For development purposes, you may need to change the Command Arguments under *Project Properties* -> *Debugging* to either the sample FBX or DB file as desired.

# Creating Your Own Converter for TexTools

Textools will automatically detect and attempt to use any new converters.  The expectations for them are as follows:

- Your converter should have a folder with the extension name it consumes at *TexTools_RootFolder*/converters/<extension>/
- This folder must contain an executable called *converter.exe*.
- That .exe file should take either a .db file (to convert into your new file format) or a file of your new file format's extension (ex. .fbx) to convert into a .db file.
- The resultant file should be placed in the same directory, labeled as *result.db* or *result.yourExtension*, as appropriate.
- STDOut will be piped to users in the log as information messages.
- STDErr will be piped to users in the log as warning messages.
- A non-zero exit code will be treated as a critical error, and be displayed as such to the end user.
- An exit code of zero will be treated as a success.
  
By default, your executable will be listed as both an importer and exporter; this may be customizable at a future date.

# DB Files

Textools communicates model data with its external exporters via a simple SQLite DB format.  The exact structure of this DB format is available in this project's TT_FBX/src/res/sq/ directory, and/or in the main xivModdingFramework project's SQL directory.
- Some data points are optional, and/or are ignored on import into TexTools.  For example, Skeleton data is immutable and ignored on import back into TexTools.
