/*-----------------------------------------.---------------------------------.
| Filename: DirectoryLoader.h              | Directory loader                |
| Author  : Francis Maes                   |                                 |
| Started : 10/11/2012 19:05               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef OIL_CORE_LOADER_DIRECTORY_H_
# define OIL_CORE_LOADER_DIRECTORY_H_

# include <oil/Core/Loader.h>
# include <oil/Core/String.h>

namespace lbcpp
{

class DirectoryLoader : public Loader
{
public:
  virtual string getFileExtensions() const
    {return string::empty;}

  virtual ClassPtr getTargetClass() const
    {return directoryClass;}

  virtual bool canUnderstand(ExecutionContext& context, const juce::File& file) const
    {return file.isDirectory();}

  virtual ObjectPtr loadFromFile(ExecutionContext& context, const juce::File& file) const
    {return new Directory(file);}
};

}; /* namespace lbcpp */

#endif // OIL_CORE_LOADER_DIRECTORY_H_
