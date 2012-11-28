/*-----------------------------------------.---------------------------------.
| Filename: DirectoryFileStream.h          | Stream of directory child files |
| Author  : Francis Maes                   |                                 |
| Started : 12/07/2010 12:03               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_DATA_STREAM_DIRECTORY_FILES_H_
# define LBCPP_DATA_STREAM_DIRECTORY_FILES_H_

# include <lbcpp/Data/Stream.h>

namespace lbcpp
{

class DirectoryFileStream : public Stream
{
public:
  DirectoryFileStream(ExecutionContext& context, const juce::File& directory, const string& wildCardPattern = T("*"), bool searchFilesRecursively = false)
    : Stream(context), directory(directory), wildCardPattern(wildCardPattern), searchFilesRecursively(searchFilesRecursively)
    {initialize();}

  DirectoryFileStream() {}

  virtual ClassPtr getElementsType() const
    {return fileClass;}

  virtual bool rewind()
    {nextFileIterator = files.begin(); return true;}

  virtual bool isExhausted() const
    {return nextFileIterator == files.end();}

  virtual ObjectPtr next()
  {
    if (isExhausted())
      return ObjectPtr();
    juce::File file(*nextFileIterator);
    ++nextFileIterator;
    ++position;
    return File::create(file);
  }

  static void findChildFiles(const juce::File& directory, const string& wildCardPattern, bool searchRecursively, std::set<string>& res)
  {
    juce::OwnedArray<juce::File> files;
    directory.findChildFiles(files, juce::File::findFiles, searchRecursively, wildCardPattern);
    for (int i = 0; i < files.size(); ++i)
      res.insert(files[i]->getFullPathName());
  }

  virtual ProgressionStatePtr getCurrentPosition() const
    {return new ProgressionState((double)this->position, (double)files.size(), T("Files"));}

private:
  juce::File directory;
  string wildCardPattern;
  bool searchFilesRecursively;

  std::set<string> files;
  std::set<string>::const_iterator nextFileIterator;
  size_t position;

  void initialize()
  {
    files.clear();    
    findChildFiles(directory, wildCardPattern, searchFilesRecursively, files);
    nextFileIterator = files.begin();
    position = 0;
  }
};

}; /* namespace lbcpp */

#endif // !LBCPP_DATA_STREAM_DIRECTORY_FILES_H_