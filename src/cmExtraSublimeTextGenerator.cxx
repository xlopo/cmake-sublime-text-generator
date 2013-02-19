/*============================================================================
  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "cmExtraSublimeTextGenerator.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmake.h"
#include "cmSourceFile.h"
#include "cmGeneratedFileStream.h"
#include "cmTarget.h"

#include <cmsys/SystemTools.hxx>

/* Some useful URLs:
Homepage:
http://www.sublimetext.com/

File format docs:
http://www.sublimetext.com/docs/2/projects.html
http://www.sublimetext.com/docs/build
http://docs.sublimetext.info/en/latest/reference/build_systems.html

Discussion:

*/

//----------------------------------------------------------------------------
cmExtraSublimeTextGenerator::cmExtraSublimeTextGenerator()
: cmExternalMakefileProjectGenerator()
{
  // We are certainly going to want to support more in the future
  this->SupportedGlobalGenerators.push_back("Unix Makefiles");
}

void cmExtraSublimeTextGenerator
::GetDocumentation(cmDocumentationEntry& entry, const char*) const
{
  entry.Name = this->GetName();
  entry.Brief = "Generates Sublime Text project files.";
  entry.Full =
    "A project file for Sublime Text will be created in the top directory "
    "and in every subdirectory which features a CMakeLists.txt file "
    "containing a PROJECT() call. "
    "Additionally a hierarchy of makefiles is generated into the "
    "build tree.  The appropriate make program can build the project through "
    "the default make target. "
    "Furthermore, clean, depend, rebuild_cache, and any CMakeLists.txt "
    "defined targets are also included as a build system.";
}

void cmExtraSublimeTextGenerator::Generate()
{
// for each sub project in the project create a Sublime Text project
for (std::map<cmStdString, std::vector<cmLocalGenerator*> >::const_iterator
     it = this->GlobalGenerator->GetProjectMap().begin();
    it!= this->GlobalGenerator->GetProjectMap().end();
    ++it)
  {
  // create a project file
  this->CreateProjectFile(it->second);
  }
}

static std::string escapeStringForJSON(const std::string &str)
{
  std::string outStr = "";

  for (unsigned int i=0; i < str.size(); i++) {
    if (str[i] == '\\' || str[i] == '"')
        outStr += '\\';
    outStr += str[i];
  }

  return outStr;
}

/* Nice utility classes to help with file structure */
struct SublimeTextProject {
    /* Folders section object */
    struct Folder {
        std::string path;
        std::vector<std::string> folder_exclude_patterns;
        std::vector<std::string> file_exclude_patterns;

        void SetPath(std::string folder_path);
        void AddFolderExcludePattern(std::string pattern);
        void AddFileExcludePattern(std::string pattern);
        std::string GenerateString();
    };

    // build_systems section object
    struct BuildSystem {
        std::string name;
        std::string working_dir;
        std::vector<std::string> cmd;
        bool shell;

        BuildSystem() : shell(false){}

        // Set whether the build should be executed within the
        // system shell (e.g. Bash)
        void SetShell(bool shell);
        // The the name of the build system as seen in the tools sub-menu
        void SetName(std::string name);
        // Set where the Makefile resides/gets executed
        void SetWorkingDirectory(std::string dir);
        // Add the build command and its arguments
        void AddToCommand(std::string cmd_part);
        // Generate build_systems tring for the Project file
        std::string GenerateString();
    };

    std::string filename;
    std::string name;

    std::vector<SublimeTextProject::Folder> folders;
    std::vector<SublimeTextProject::BuildSystem> build_systems;

    void SetName(std::string projectName);
    /* Add a folder based off the given cmMakefile */
    void AddFolder(const cmMakefile* makeFile);
    /* Add an existing folder tot he project */
    void AddFolder(SublimeTextProject::Folder &folder);
    /* Add a build system */
    void AddBuildSystem(SublimeTextProject::BuildSystem &bs);
    /* Geerate the ful config text */
    std::string GenerateProjectText();
};

void SublimeTextProject::Folder
::SetPath(std::string folder_path)
{
    path = folder_path;
}

void SublimeTextProject::Folder
::AddFolderExcludePattern(std::string pattern)
{
    folder_exclude_patterns.push_back(pattern);
}

void SublimeTextProject::Folder
::AddFileExcludePattern(std::string pattern)
{
    file_exclude_patterns.push_back(pattern);
}

void SublimeTextProject::BuildSystem
::SetShell(bool in_shell)
{
    shell = in_shell;
}

void SublimeTextProject::BuildSystem
::SetName(std::string bs_name)
{
    name = bs_name;
}

void SublimeTextProject::BuildSystem
::SetWorkingDirectory(std::string dir)
{
    working_dir = dir;
}

void SublimeTextProject::BuildSystem
::AddToCommand(std::string part)
{
    cmd.push_back(part);
}

std::string SublimeTextProject::BuildSystem
::GenerateString()
{
  // escape the strings for JSON as needed
  std::string jName = escapeStringForJSON(name);
  std::string jWorkingDir = escapeStringForJSON(working_dir);

  // construct the textual representation of a build_system
  std::string bss = "";
  bss += "        {\n";
  bss += "            \"name\": \"" + jName + "\",\n";
  bss += "            \"working_dir\": \"" + jWorkingDir + "\",\n";

  if (shell)
    bss += "            \"shell\": true,\n";

  bss += "            \"cmd\": [ ";

  // turn the command into a JSON array
  for(unsigned int i=0; i < cmd.size(); i++)
    {
      std::string jCommand = escapeStringForJSON(cmd[i]);
      bss += "\"" + jCommand + "\"";

      if (i+1 < cmd.size())
          bss +=", ";
    }
  bss += " ]\n";
  bss += "        }";

  return bss;
}

void SublimeTextProject::SetName(std::string projectName)
{
  name = projectName;
}

void SublimeTextProject::AddFolder(const cmMakefile* makeFile)
{
    // Adding a folder based on the given makeFile
    cmStdString homeDir = makeFile->GetHomeDirectory();

    SublimeTextProject::Folder folder;
    folder.SetPath(homeDir);

    /* Ignore cached generated cmake files */
    folder.AddFileExcludePattern("CMakeCache.txt");
    folder.AddFileExcludePattern("cmake_install.cmake");

    /* Ignore CMakeFiles folder */
    folder.AddFolderExcludePattern("CMakeFiles");

    /* Add it to the master folder list */
    AddFolder(folder);
}

void SublimeTextProject::AddBuildSystem(SublimeTextProject::BuildSystem &bs)
{
  build_systems.push_back(bs);
}

/* Add a folder to this Projects folder list */
void SublimeTextProject::AddFolder(SublimeTextProject::Folder &folder)
{
  folders.push_back(folder);
}

std::string SublimeTextProject::GenerateProjectText()
{
    std::string jName = escapeStringForJSON(name);

    std::string text = "{\n";

    // Set the project name
    text += "    \"name\": \"" + jName + "\",\n";

    /* Begin "folders" section */
    text += "    \"folders\":\n";
    text += "    [\n";
    for(unsigned int i=0; i < folders.size(); i++)
      {
        text += folders[i].GenerateString();

        /* Separate folders by comma. Cannot have trailing comma. */
        if (i+1 != folders.size())
            text += ",";
        text += "\n";
      }
    text += "    ],\n";
    /* End "folders" section */

    /* Begin "build_systems" section */
    text += "    \"build_systems\":\n";
    text += "    [\n";
    /* create a unique build system for each target in the Makefile
     * as well the default Make action and "clean"
     */
    for(unsigned int i=0; i < build_systems.size(); i++)
      {
        SublimeTextProject::BuildSystem &bs = build_systems[i];

        text += bs.GenerateString();

        if (i+1 != build_systems.size())
            text += ",";
        text += "\n";
      }
    text += "    ]\n";
    text += "}\n";

    return text;
}

std::string SublimeTextProject::Folder
::GenerateString()
{
    std::string jPath = escapeStringForJSON(path);

    std::string fs = "";

    fs += "        {\n";
    fs += "            \"path\": \"" + jPath + "\",\n";

    /* Add the folder we are going to ignore */
    fs += "            \"folder_exclude_patterns\": [ ";
    for (unsigned int i=0; i < folder_exclude_patterns.size(); i++)
      {
        std::string jPattern = escapeStringForJSON(folder_exclude_patterns[i]);
        fs += "\"" + jPattern + "\"";

        if (i+1 < folder_exclude_patterns.size())
            fs +=", ";
      }
    fs += " ],\n";

    /* Add the folder we are going to ignore */
    fs += "            \"file_exclude_patterns\": [ ";
    for (unsigned int i=0; i < file_exclude_patterns.size(); i++)
      {
        std::string jPattern = escapeStringForJSON(file_exclude_patterns[i]);
        fs += "\"" + jPattern + "\"";

        if (i+1 < file_exclude_patterns.size())
            fs +=", ";
      }
    fs += " ]\n";

    fs += "        }";

    return fs;
}


bool cmExtraSublimeTextGenerator
::UsingNCursesGUI(cmMakefile *makeFile)
{
  std::string editCommand(makeFile->GetDefinition("CMAKE_EDIT_COMMAND"));
  size_t foundCCMake = editCommand.find("ccmake");
  return foundCCMake != std::string::npos;
}

/* create the project file */
void cmExtraSublimeTextGenerator::CreateProjectFile(
                                     const std::vector<cmLocalGenerator*>& lgs)
{
  const cmMakefile* mf=lgs[0]->GetMakefile();
  std::string outputDir=mf->GetStartOutputDirectory();
  std::string projectName=mf->GetProjectName();


  CreateNewProjectFile(lgs, projectName, outputDir);
}

/* Create a project file and associated build_systems and folders */
void cmExtraSublimeTextGenerator
::CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs,
                            const std::string& projectName,
                            const std::string& outputDir)
{
  SublimeTextProject stp;
  stp.SetName(projectName);

  // set the name of the project file
  stp.filename = outputDir + "/" + projectName + ".sublime-project";

  // Add home directory of all make files as folder to the project
  for (std::vector<cmLocalGenerator*>::const_iterator lg=lgs.begin();
       lg!=lgs.end(); lg++)
    {
      cmMakefile *makeFile = (*lg)->GetMakefile();
      stp.AddFolder(makeFile);
    }

  // create build systems
  for (std::vector<cmLocalGenerator*>::const_iterator lg=lgs.begin();
       lg!=lgs.end(); lg++)
    {
      cmMakefile* makeFile=(*lg)->GetMakefile();
      std::string make = makeFile->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
      std::string makeProjectName = makeFile->GetProjectName();

      // Add default build targets
      // make
      SublimeTextProject::BuildSystem defaultBS;
      defaultBS.SetName(makeProjectName + ": default");
      defaultBS.SetWorkingDirectory(outputDir);
      defaultBS.AddToCommand(make);
      stp.AddBuildSystem(defaultBS);

      // make clean
      SublimeTextProject::BuildSystem cleanBS;
      cleanBS.SetName(makeProjectName + ": clean");
      cleanBS.SetWorkingDirectory(outputDir);
      cleanBS.AddToCommand(make);
      cleanBS.AddToCommand("clean");
      stp.AddBuildSystem(cleanBS);

      // make depend
      SublimeTextProject::BuildSystem dependBS;
      dependBS.SetName(makeProjectName + ": depend");
      dependBS.SetWorkingDirectory(outputDir);
      dependBS.AddToCommand(make);
      dependBS.AddToCommand("depend");
      stp.AddBuildSystem(dependBS);

      // Add user defined / other build targets
      cmTargets &targets = makeFile->GetTargets();
      for(cmTargets::const_iterator t = targets.begin();
        t != targets.end(); t++)
        {
          const cmTarget &target = t->second;
          SublimeTextProject::BuildSystem bs;
          std::string target_name(target.GetName());

         // ncurses gui doesn't work in Sublime Text's Console
         if (target_name == "edit_cache" && UsingNCursesGUI(makeFile))
            continue;

          // Prepend the make project name to give menu context
          std::string bs_name = makeProjectName + ": " + target_name;

          // Create the build system
          bs.SetWorkingDirectory(makeFile->GetCurrentDirectory());
          bs.SetName(bs_name);
          bs.AddToCommand(make);
          bs.AddToCommand(target.GetName());

          // add it to the list of build systems
          stp.AddBuildSystem(bs);
        }

    }

    // Output our project file
    cmGeneratedFileStream fout(stp.filename.c_str());
    fout << stp.GenerateProjectText();
    fout.close();
}
