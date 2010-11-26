/*-----------------------------------------.---------------------------------.
| Filename: Class.cpp                      | Class Introspection             |
| Author  : Francis Maes                   |                                 |
| Started : 25/08/2010 02:16               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include <lbcpp/Core/Type.h>
#include <lbcpp/Core/Variable.h>
#include <lbcpp/Core/XmlSerialisation.h>
#include <lbcpp/Data/Vector.h>
#include <map>
using namespace lbcpp;

/*
** Class
*/
String Class::toString() const
{
  String res = getName();
  res += T(" = {");
  size_t n = getObjectNumVariables();
  for (size_t i = 0; i < n; ++i)
  {
    res += getObjectVariableType(i)->getName() + T(" ") + getObjectVariableName(i);
    if (i < n - 1)
      res += T(", ");
  }
  res += T("}");
  return res;
}

int Class::compare(const VariableValue& value1, const VariableValue& value2) const
{
  ObjectPtr object1 = value1.getObject();
  ObjectPtr object2 = value2.getObject();
  if (!object1)
    return object2 ? -1 : 0;
  if (!object2)
    return 1;
  return object1->compare(object2);
}

VariableValue Class::createFromString(ExecutionContext& context, const String& value) const
{
  VariableValue res = create(context);
  if (isMissingValue(res))
  {
    context.errorCallback(T("Class::createFromString"), T("Could not create instance of ") + getName().quoted());
    return getMissingValue();
  }
  return res.getObject()->loadFromString(context, value) ? res : getMissingValue();
}

VariableValue Class::createFromXml(XmlImporter& importer) const
{
  VariableValue res = create(importer.getContext());
  if (isMissingValue(res))
  {
    importer.errorMessage(T("Class::createFromXml"), T("Could not create instance of ") + getName().quoted());
    return getMissingValue();
  }
  return res.getObject()->loadFromXml(importer) ? res : getMissingValue();
}

void Class::saveToXml(XmlExporter& exporter, const VariableValue& value) const
{
  ObjectPtr object = value.getObject();
  jassert(object);
  object->saveToXml(exporter);
}

ClassPtr Class::getClass() const
  {return classClass;}

/*
** DefaultClass
*/
DefaultClass::DefaultClass(const String& name, TypePtr baseClass)
  : Class(name, baseClass)
{
}

DefaultClass::DefaultClass(const String& name, const String& baseClass)
  : Class(name, silentExecutionContext->getType(baseClass))
{
}

DefaultClass::DefaultClass(TemplateTypePtr templateType, const std::vector<TypePtr>& templateArguments, TypePtr baseClass)
  : Class(templateType, templateArguments, baseClass) {}

void DefaultClass::clearVariables()
{
  variables.clear();
}

void DefaultClass::addVariable(ExecutionContext& context, const String& typeName, const String& name, const String& shortName, const String& description)
{
  TypePtr type;
  if (templateType)
    type = templateType->instantiateTypeName(context, typeName, templateArguments);
  else
    type = context.getType(typeName);
  if (type)
    addVariable(context, type, name, shortName, description);
}

void DefaultClass::addVariable(ExecutionContext& context, TypePtr type, const String& name, const String& shortName, const String& description)
{
  if (!type || name.isEmpty())
  {
    context.errorCallback(T("Class::addVariable"), T("Invalid type or name"));
    return;
  }
  if (findObjectVariable(name) >= 0)
  {
    context.errorCallback(T("Class::addVariable"), T("Another variable with name '") + name + T("' already exists"));
    return;
  }
  variablesMap[name] = variables.size();
  VariableInfo info;
  info.type = type;
  info.name = name;
  info.shortName = shortName.isNotEmpty() ? shortName : name;
  info.description = description.isNotEmpty() ? description : name;
  variables.push_back(info);
}

size_t DefaultClass::getObjectNumVariables() const
{
  size_t n = baseType->getObjectNumVariables();
  return n + variables.size();
}

TypePtr DefaultClass::getObjectVariableType(size_t index) const
{
  size_t n = baseType->getObjectNumVariables();
  if (index < n)
    return baseType->getObjectVariableType(index);
  index -= n;
  
  jassert(index < variables.size());
  return variables[index].type;
}

String DefaultClass::getObjectVariableName(size_t index) const
{
  size_t n = baseType->getObjectNumVariables();
  if (index < n)
    return baseType->getObjectVariableName(index);
  index -= n;
  
  jassert(index < variables.size());
  return variables[index].name;
}

String DefaultClass::getObjectVariableShortName(size_t index) const
{
  size_t n = baseType->getObjectNumVariables();
  if (index < n)
    return baseType->getObjectVariableShortName(index);
  index -= n;
  
  jassert(index < variables.size());
  return variables[index].shortName;
}

String DefaultClass::getObjectVariableDescription(size_t index) const
{
  size_t n = baseType->getObjectNumVariables();
  if (index < n)
    return baseType->getObjectVariableDescription(index);
  index -= n;
  
  jassert(index < variables.size());
  return variables[index].description;
}

void DefaultClass::deinitialize()
{
  variables.clear();
  Class::deinitialize();
}

int DefaultClass::findObjectVariable(const String& name) const
{
  std::map<String, size_t>::const_iterator it = variablesMap.find(name);
  if (it != variablesMap.end())
    return (int)(baseType->getObjectNumVariables() + it->second);
  return baseType->findObjectVariable(name);
}