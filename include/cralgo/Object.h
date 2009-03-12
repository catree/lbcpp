/*-----------------------------------------.---------------------------------.
| Filename: Object.h                       | A base class for serializable   |
| Author  : Francis Maes                   |  objects                        |
| Started : 06/03/2009 17:04               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/
                               
#ifndef CRALGO_OBJECT_H_
# define CRALGO_OBJECT_H_

# include "ContainerTraits.h"
# include "ReferenceCountedObject.h"
# include "ErrorHandler.h"

namespace cralgo
{

class Object;
typedef ReferenceCountedObjectPtr<Object> ObjectPtr;

class Object : public ReferenceCountedObject
{
public:
  virtual ~Object() {}
  
  typedef Object* (*Constructor)();
    
  static void declare(const std::string& className, Constructor constructor);
  static Object* create(const std::string& className);
  
  static ObjectPtr loadFromStream(std::istream& istr);
  static ObjectPtr loadFromFile(const std::string& fileName);
  
  template<class T>
  static ReferenceCountedObjectPtr<T> loadFromStreamCast(std::istream& istr)
    {return checkCast<T>("Object::createFromStreamCast", loadFromStream(istr));}

  template<class T>
  static ReferenceCountedObjectPtr<T> loadFromFileCast(const std::string& fileName)
    {return checkCast<T>("Object::createFromFileCast", loadFromFile(fileName));}

  std::string getClassName() const;
  virtual std::string getName() const
    {return "";}
    
  virtual std::string toString() const
    {return "";}
    
  bool saveToFile(const std::string& fileName) const;
  void saveToStream(std::ostream& ostr) const;
  
protected:
  template<class T>
  static ReferenceCountedObjectPtr<T> checkCast(const std::string& where, ObjectPtr object)
  {
    ReferenceCountedObjectPtr<T> res;
    if (object)
    {
      
      res = object.dynamicCast<T>();
      if (!res)
        ErrorHandler::error(where, "Could not cast object into '" + cralgo::toString(typeid(*res)) + "'");
    }
    return res;
  }

  virtual bool load(std::istream& istr) {return true;}
  virtual void save(std::ostream& ostr) const {}
};

template<class Type>
struct ObjectConstructor_
  {static Object* construct() {return new Type();} };
#define DECLARE_CRALGO_CLASS(Name) \
  cralgo::Object::declare(cralgo::toString(typeid(Name)), cralgo::ObjectConstructor_<Name>::construct)

template<class T>
struct ObjectPtrTraits
{
public:
  static inline std::string toString(const T* value)
    {return value ? value->getClassName() : "null";}
  static inline void write(std::ostream& ostr, const T* value)
    {assert(value); value->saveToStream(ostr);}
  static inline bool read(std::istream& istr, T* result)
    {result = Object::loadFromStreamCast<T>(istr); return result != NULL;}
};

template<class T>
struct ObjectSharedPtrTraits
{
public:
  static inline std::string toString(const ReferenceCountedObjectPtr<T> value)
    {return value ? value->toString() : "null";}
  static inline void write(std::ostream& ostr, const ReferenceCountedObjectPtr<T> value)
    {assert(value); value->saveToStream(ostr);}
  static inline bool read(std::istream& istr, ReferenceCountedObjectPtr<T>& result)
    {result = Object::loadFromStreamCast<T>(istr); return result;}
};

template<>
struct Traits<Object*> : public ObjectPtrTraits<Object> {};
template<>
struct Traits<ObjectPtr> : public ObjectPtrTraits<Object> {};

// todo: move somewhere into implementation
namespace impl
{ 
  template<class ExactType>
  struct Object
  {
    std::string toString() const {return "";}
    
    void save(std::ostream& ostr) const {}
    bool load(std::istream& istr) {return true;}
  };
}; /* namespace impl */

}; /* namespace cralgo */

#endif // !CRALGO_OBJECT_H_
