/*-----------------------------------------.---------------------------------.
| Filename: FeatureDictionary.h            | A dictionary of feature and     |
| Author  : Francis Maes                   | feature-scope names             |
| Started : 06/03/2009 17:06               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
                               
#ifndef LBCPP_FEATURE_DICTIONARY_H_
# define LBCPP_FEATURE_DICTIONARY_H_

# include "ObjectPredeclarations.h"
# include <map>

namespace lbcpp
{

class StringDictionary : public Object
{
public:
  void clear()
    {stringToIndex.clear(); indexToString.clear();}

  size_t getNumElements() const
    {return (unsigned)indexToString.size();}

  bool exists(size_t index) const;
  std::string getString(size_t index) const;
  
  // returns -1 if not found
  int getIndex(const std::string& str) const;
  
  size_t add(const std::string& str);

  friend std::ostream& operator <<(std::ostream& ostr, const StringDictionary& strings);

  /*
  ** Object
  */
  virtual std::string toString() const
    {std::ostringstream ostr; ostr << *this; return ostr.str();}

  virtual void save(std::ostream& ostr) const;
  virtual bool load(std::istream& istr);  

protected:
  typedef std::map<std::string, size_t> StringToIndexMap;
  typedef std::vector<std::string> StringVector;
 
  StringToIndexMap stringToIndex;
  StringVector indexToString;
};

typedef ReferenceCountedObjectPtr<StringDictionary> StringDictionaryPtr;

class FeatureDictionary : public Object
{
public:
  FeatureDictionary(const std::string& name, StringDictionaryPtr features, StringDictionaryPtr scopes);
  FeatureDictionary(const std::string& name = "unnamed");
    
  bool empty() const
    {return getNumFeatures() == 0 && getNumScopes() == 0;}
  
  /*
  ** Features
  */
  StringDictionaryPtr getFeatures()
    {return featuresDictionary;}
  
  size_t getNumFeatures() const
    {return featuresDictionary ? featuresDictionary->getNumElements() : 0;}
    
  size_t addFeature(const std::string& identifier)
    {assert(featuresDictionary); return featuresDictionary->add(identifier);}
    
  /*
  ** Scopes
  */
  StringDictionaryPtr getScopes()
    {return scopesDictionary;}

  size_t getNumScopes() const
    {return scopesDictionary ? scopesDictionary->getNumElements() : 0;}
    
  void addScope(const std::string& name, FeatureDictionaryPtr subDictionary)
  {
    FeatureDictionaryPtr d = getSubDictionary(getScopes()->add(name), subDictionary);
    assert(d == subDictionary);
  }
  
  /*
  ** Related dictionaries
  */
  void setSubDictionary(size_t index, FeatureDictionaryPtr dictionary)
    {if (subDictionaries.size() < index + 1) subDictionaries.resize(index + 1); subDictionaries[index] = dictionary;}
    
  const FeatureDictionaryPtr getSubDictionary(size_t index) const
    {assert(index < subDictionaries.size()); return subDictionaries[index];}
    
  FeatureDictionaryPtr getSubDictionary(size_t index, FeatureDictionaryPtr defaultValue = FeatureDictionaryPtr());
  
  FeatureDictionaryPtr getSubDictionary(const std::string& name, FeatureDictionaryPtr defaultValue = FeatureDictionaryPtr())
    {assert(scopesDictionary); return getSubDictionary(scopesDictionary->getIndex(name), defaultValue);}

  FeatureDictionaryPtr getDictionaryWithSubScopesAsFeatures();
  
  /*
  ** Object
  */
  virtual std::string getName() const
    {return name;}
    
  virtual std::string toString() const
  {
    return "Features: " + lbcpp::toString(*featuresDictionary) + "\n"
           "Scopes: " + lbcpp::toString(*scopesDictionary) + "\n";
  }

  virtual bool load(std::istream& istr);
  virtual void save(std::ostream& ostr);
  
private:
  std::string name;
  StringDictionaryPtr featuresDictionary;
  StringDictionaryPtr scopesDictionary;
  std::vector<FeatureDictionaryPtr> subDictionaries;
  FeatureDictionaryPtr dictionaryWithSubScopesAsFeatures;

  void enumerateUniqueDictionaries(std::map<FeatureDictionaryPtr, size_t>& indices, std::vector<FeatureDictionaryPtr>& res);
};

}; /* namespace lbcpp */

#endif // !LBCPP_STRING_DICTIONARY_H_
