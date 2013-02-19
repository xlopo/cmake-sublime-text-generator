/*============================================================================
  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef cmExtraSublimeTextGenerator_h
#define cmExtraSublimeTextGenerator_h

#include "cmExternalMakefileProjectGenerator.h"

class cmLocalGenerator;
class cmMakefile;
class cmTarget;
class cmGeneratedFileStream;

/** \class cmExtraSublimeTextGenerator
 * \brief Write Sublime Text project files for Makefile based projects
 */
class cmExtraSublimeTextGenerator : public cmExternalMakefileProjectGenerator
{
public:
  cmExtraSublimeTextGenerator();
  virtual const char* GetName() const
                       { return cmExtraSublimeTextGenerator::GetActualName(); }
  static const char* GetActualName()                 { return "Sublime Text"; }
  static cmExternalMakefileProjectGenerator* New()
                                    { return new cmExtraSublimeTextGenerator; }
  /** Get the documentation entry for this generator.  */
  virtual void GetDocumentation(cmDocumentationEntry& entry,
                                const char* fullName) const;
  virtual void Generate();

private:
  bool UsingNCursesGUI(cmMakefile *makefile);
  void CreateProjectFile(const std::vector<cmLocalGenerator*>& lgs);
  void CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs,
                            const std::string& projectName,
                            const std::string& outputDir);
};

#endif
