/*-----------------------------------------.---------------------------------.
| Filename: IntrospectionGenerator.cpp     | Introspection Generator         |
| Author  : Francis Maes                   |                                 |
| Started : 18/08/2010 17:33               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#include "../extern/juce/juce_amalgamated.h"
#include <map>
#include <vector>
#include <iostream>
#include <set>

static File inputFile;

class CppCodeGenerator
{
public:
  CppCodeGenerator(XmlElement* xml, OutputStream& ostr) : xml(xml), ostr(ostr)
  {
    fileName = xml->getStringAttribute(T("name"), T("???"));
    directoryName = xml->getStringAttribute(T("directory"), String::empty);
  }

  void generate()
  {
    indentation = 0;
    generateHeader();
    newLine();

    String namespaceName = xml->getStringAttribute(T("namespace"), T("lbcpp"));
    newLine();
    openScope(T("namespace ") + namespaceName);

    generateCodeForChildren(xml);
    newLine();
    generateLibraryClass();
    //generateFooter();
    newLine();

    if (xml->getBoolAttribute(T("dynamic")))
    {
      generateDynamicLibraryFunctions();
      newLine();
    }

    newLine();
    closeScope(T("; /* namespace ") + namespaceName + T(" */\n"));
  }

protected:
  static String xmlTypeToCppType(const String& typeName)
    {return typeName.replaceCharacters(T("[]"), T("<>"));}

  static String typeToRefCountedPointerType(const String& typeName)
  {
    String str = xmlTypeToCppType(typeName);
    int i = str.indexOfChar('<');
    if (i >= 0)
      str = str.substring(0, i);
    return str + T("Ptr");
  }

  static String replaceFirstLettersByLowerCase(const String& str)
  {
    if (str.isEmpty())
      return String::empty;
    int numUpper = 0;
    for (numUpper = 0; numUpper < str.length(); ++numUpper)
      if (!CharacterFunctions::isUpperCase(str[numUpper]) &&
          !CharacterFunctions::isDigit(str[numUpper]))
        break;

    if (numUpper == 0)
      return str;

    String res = str;
    if (numUpper == 1)
      res[0] = CharacterFunctions::toLowerCase(res[0]);
    else
      for (int i = 0; i < numUpper - 1; ++i)
        res[i] = CharacterFunctions::toLowerCase(res[i]);

    return res;
  }

  void generateCodeForChildren(XmlElement* xml)
  {
    for (XmlElement* elt = xml->getFirstChildElement(); elt; elt = elt->getNextElement())
    {
      String tag = elt->getTagName();
      if (tag == T("class"))
        generateClassDeclaration(elt, NULL);
      else if (tag == T("template"))
      {
        for (XmlElement* cl = elt->getFirstChildElement(); cl; cl = cl->getNextElement())
          if (cl->getTagName() == T("class"))
          {
            generateClassDeclaration(cl, elt);
            newLine();
          }
        generateTemplateClassDeclaration(elt);
      }
      else if (tag == T("enumeration"))
        generateEnumerationDeclaration(elt);
      else if (tag == T("uicomponent"))
#ifdef LBCPP_USER_INTERFACE
        declarations.push_back(Declaration::makeUIComponent(currentNamespace, elt->getStringAttribute(T("name")), xmlTypeToCppType(elt->getStringAttribute(T("type")))));
#else
        {} // ignore
#endif // LBCPP_USER_INTERFACE
      else if (tag == T("code"))
        generateCode(elt);
      else if (tag == T("namespace"))
      {
        String name = elt->getStringAttribute(T("name"), T("???"));
        String previousNamespace = currentNamespace;
        if (currentNamespace.isNotEmpty())
          currentNamespace += T("::");
        currentNamespace += name;

        writeLine(T("namespace ") + name + T(" {"));
        generateCodeForChildren(elt);
        writeLine(T("}; /* namespace ") + name + T(" */"));

        currentNamespace = previousNamespace;
      }
      else if (tag == T("import") || tag == T("include"))
        continue;
      else
        std::cerr << "Warning: unrecognized tag: " << (const char* )tag << std::endl;
    }
  }

  /*
  ** Header
  */
  void generateHeader()
  {
    // header
    ostr << "/* ====== Introspection for file '" << fileName << "', generated on "
      << Time::getCurrentTime().toString(true, true, false) << " ====== */";
    writeLine(T("#include \"precompiled.h\""));
    writeLine(T("#include <oil/Core.h>"));
    writeLine(T("#include <oil/Lua/Lua.h>"));
    writeLine(T("#include <oil/library.h>"));

    OwnedArray<File> headerFiles;
    File directory = inputFile.getParentDirectory();
    directory.findChildFiles(headerFiles, File::findFiles, false, T("*.h"));
    directory.findChildFiles(headerFiles, File::findFiles, false, T("*.hpp"));
    std::set<String> sortedFiles;
    for (int i = 0; i < headerFiles.size(); ++i)
    {
      String path = directoryName;
      if (path.isNotEmpty())
        path += T("/");
      path += headerFiles[i]->getRelativePathFrom(directory).replaceCharacter('\\', '/');
      sortedFiles.insert(path);
    }
    for (std::set<String>::const_iterator it = sortedFiles.begin(); it != sortedFiles.end(); ++it)
      writeLine(T("#include ") + it->quoted());

    forEachXmlChildElementWithTagName(*xml, elt, T("include"))
      generateInclude(elt);
  }

  /*
  ** Include
  */
  void generateInclude(XmlElement* xml)
  {
    writeLine(T("#include ") + xml->getStringAttribute(T("file"), T("???")).quoted());
  }

  /*
  ** Enumeration
  */
  void generateEnumValueInInitialize(XmlElement* xml)
  {
    String name = xml->getStringAttribute(T("name"), T("???"));
    String oneLetterCode = xml->getStringAttribute(T("oneLetterCode"), String::empty);
    String threeLettersCode = xml->getStringAttribute(T("threeLettersCode"), String::empty);
    writeLine(T("addElement(context, T(") + name.quoted() + T("), T(") + oneLetterCode.quoted() + T("), T(") + threeLettersCode.quoted() + T("));"));
  }

  void generateEnumerationDeclaration(XmlElement* xml)
  {
    String enumName = xml->getStringAttribute(T("name"), T("???"));

    Declaration declaration = Declaration::makeType(currentNamespace, enumName, T("Enumeration"));
    declarations.push_back(declaration);
    
    openClass(declaration.implementationClassName, T("DefaultEnumeration"));

    // constructor
    openScope(declaration.implementationClassName + T("() : DefaultEnumeration(T(") + makeFullName(enumName, true) + T("))"));
    closeScope();

    newLine();

    openScope(T("virtual bool initialize(ExecutionContext& context)"));
      forEachXmlChildElementWithTagName(*xml, elt, T("value"))
        generateEnumValueInInitialize(elt);
      writeLine(T("return DefaultEnumeration::initialize(context);"));
    closeScope();
    newLine();

    
    // custom code
    forEachXmlChildElementWithTagName(*xml, elt, T("code"))
      {generateCode(elt); newLine();}

    closeClass();

    // enum declarator
    writeLine(T("EnumerationPtr ") + declaration.cacheVariableName + T(";"));
    //writeShortFunction(T("EnumerationPtr ") + declaratorName + T("()"),
    //  T("static TypeCache cache(T(") + enumName.quoted() + T(")); return (EnumerationPtr)cache();"));
  }

  String makeFullName(const String& identifier, bool quote = false) const
  {
    String res = currentNamespace.isNotEmpty() ? currentNamespace + T("::") + identifier : identifier;
    return quote ? res.quoted() : res;
  }

  /*
  ** Class
  */
  void generateClassDeclaration(XmlElement* xml, XmlElement* templateClassXml)
  {
    XmlElement* attributesXml = (templateClassXml ? templateClassXml : xml);

    String className = (templateClassXml && !xml->hasAttribute(T("name")) ? templateClassXml : xml)->getStringAttribute(T("name"), String::empty);
    bool isAbstract = attributesXml->getBoolAttribute(T("abstract"), false);
    String classShortName = attributesXml->getStringAttribute(T("shortName"), String::empty);
    String classBaseClass = attributesXml->getStringAttribute(T("metaclass"), T("DefaultClass"));
    String metaClass = getMetaClass(classBaseClass);
    String baseClassName = xmlTypeToCppType(attributesXml->getStringAttribute(T("base"), getDefaultBaseType(metaClass)));

    bool isTemplate = (templateClassXml != NULL);
    Declaration declaration = isTemplate ? Declaration::makeTemplateClass(currentNamespace, className, metaClass) : Declaration::makeType(currentNamespace, className, metaClass);
    if (!isTemplate)
      declarations.push_back(declaration);

    openClass(declaration.implementationClassName, classBaseClass);

    // constructor
    std::vector<XmlElement* > variables;
    std::vector<XmlElement* > functions;
    if (isTemplate)
      openScope(declaration.implementationClassName + T("(TemplateClassPtr templateType, const std::vector<ClassPtr>& templateArguments, ClassPtr baseClass)")
        + T(" : ") + classBaseClass + T("(templateType, templateArguments, baseClass)"));
    else
    {
      String arguments = T("T(") + makeFullName(className, true) + T("), T(") + baseClassName.quoted() + T(")");
      openScope(declaration.implementationClassName + T("() : ") + classBaseClass + T("(") + arguments + T(")"));
    }
    if (classShortName.isNotEmpty())
      writeLine(T("shortName = T(") + classShortName.quoted() + T(");"));
    if (isAbstract)
      writeLine(T("abstractClass = true;"));
    closeScope();
    newLine();

    openScope(T("virtual bool initialize(ExecutionContext& context)"));
    
      forEachXmlChildElementWithTagName(*xml, elt, T("function"))
      {
        generateFunctionRegistrationCode(className, elt);
        functions.push_back(elt);
      }

      forEachXmlChildElementWithTagName(*xml, elt, T("variable"))
      {
        generateVariableRegistrationCode(className, elt);
        variables.push_back(elt);
      }
      writeLine(T("return ") + classBaseClass + T("::initialize(context);"));
    closeScope();
    newLine();

    // create() function
    if (classBaseClass == T("DefaultClass"))
    {
      openScope(T("virtual lbcpp::ObjectPtr createObject(ExecutionContext& context) const"));
      if (isAbstract)
      {
        writeLine(T("context.errorCallback(\"Cannot instantiate abstract class ") + className + T("\");"));
        writeLine(T("return lbcpp::ObjectPtr();"));
      }
      else
      {
        writeLine(className + T("* res = new ") + className + T("();"));
        writeLine(T("res->setThisClass(refCountedPointerFromThis(this));"));
        writeLine(T("return lbcpp::ObjectPtr(res);"));
      }
      closeScope();
      newLine();
    }

    // getStaticVariableReference() function
    if (variables.size() && !xml->getBoolAttribute(T("manualAccessors"), false) && classBaseClass == T("DefaultClass"))
    {
      // getMemberVariableValue
      openScope(T("virtual lbcpp::ObjectPtr getMemberVariableValue(const Object* __thisbase__, size_t __index__) const"));
        if (baseClassName != T("Object"))
        {
          writeLine(T("static size_t numBaseMemberVariables = baseType->getNumMemberVariables();"));
          writeLine(T("if (__index__ < numBaseMemberVariables)"));
          writeLine(T("return baseType->getMemberVariableValue(__thisbase__, __index__);"), 1);
          writeLine(T("__index__ -= numBaseMemberVariables;"));
        }
        writeLine(T("const ") + className + T("* __this__ = static_cast<const ") + className + T("* >(__thisbase__);"));
        //writeLine(T("const ClassPtr& expectedType = variables[__index__]->getType();"));
        newLine();
        openScope(T("switch (__index__)"));
          for (size_t i = 0; i < variables.size(); ++i)
          {
            String name = variables[i]->getStringAttribute(T("var"), String::empty);
            if (name.isEmpty())
              name = variables[i]->getStringAttribute(T("name"), T("???"));

            String code = T("case ") + String((int)i) + T(": return lbcpp::nativeToObject(");
            bool isEnumeration = variables[i]->getBoolAttribute(T("enumeration"), false);
            if (isEnumeration)
              code += T("(int)(__this__->") + name + T(")");
            else
              code += T("__this__->") + name;

            code += T(", variables[__index__]->getType());");
            writeLine(code, -1);
          }
          writeLine(T("default: jassert(false); return lbcpp::ObjectPtr();"), -1);
        closeScope();
      closeScope();
      newLine();

      // setMemberVariableValue
      openScope(T("virtual void setMemberVariableValue(Object* __thisbase__, size_t __index__, const lbcpp::ObjectPtr& __subValue__) const"));
        writeLine(T("if (__index__ < baseType->getNumMemberVariables())"));
        writeLine(T("{baseType->setMemberVariableValue(__thisbase__, __index__, __subValue__); return;}"), 1);
        writeLine(T("__index__ -= baseType->getNumMemberVariables();"));
        writeLine(className + T("* __this__ = static_cast<") + className + T("* >(__thisbase__);"));
        newLine();
        openScope(T("switch (__index__)"));
          for (size_t i = 0; i < variables.size(); ++i)
          {
            String name = variables[i]->getStringAttribute(T("var"), String::empty);
            if (name.isEmpty())
              name = variables[i]->getStringAttribute(T("name"), T("???"));

            String code = T("case ") + String((int)i) + T(": lbcpp::objectToNative(defaultExecutionContext(), ");

            bool isEnumeration = variables[i]->getBoolAttribute(T("enumeration"), false);
            if (isEnumeration)
              code += T("(int& )(__this__->") + name + T(")");
            else
              code += T("__this__->") + name;
            code += T(", __subValue__); break;");
            writeLine(code, -1);
          }
          writeLine(T("default: jassert(false);"), -1);
        closeScope();
      closeScope();
    }

    // function forwarders
    for (size_t i = 0; i < functions.size(); ++i)
    {
      XmlElement* elt = functions[i];
      String functionName = elt->getStringAttribute(T("name"));
      writeShortFunction(T("static int ") + functionName + T("Forwarder(lua_State* L)"),
                         T("LuaState state(L); return ") + className + T("::") + functionName + T("(state);")); 
    }

    forEachXmlChildElementWithTagName(*xml, elt, T("code"))
      {generateCode(elt); newLine();}

    closeClass();

    // class declarator
    if (!isTemplate)
    {
      writeLine(metaClass + T("Ptr ") + declaration.cacheVariableName + T(";"));

      // class constructors
      forEachXmlChildElementWithTagName(*xml, elt, T("constructor"))
        generateClassConstructorMethod(elt, className, baseClassName);
    }
  }

  void generateVariableRegistrationCode(const String& className, XmlElement* xml)
  {
    String type = xmlTypeToCppType(xml->getStringAttribute(T("type"), T("???")));
    String name = xml->getStringAttribute(T("name"), T("???"));
    String shortName = xml->getStringAttribute(T("shortName"), String::empty);
    String description = xml->getStringAttribute(T("description"), String::empty);
    String typeArgument = (type == className ? T("this") : T("T(") + type.quoted() + T(")"));
    
    String arguments = typeArgument + T(", T(") + name.quoted() + T(")");
    arguments += T(", ");
    arguments += shortName.isEmpty() ? T("lbcpp::string::empty") : T("T(") + shortName.quoted() + T(")");
    arguments += T(", ");
    arguments += description.isEmpty() ? T("lbcpp::string::empty") : T("T(") + description.quoted() + T(")");
    
    if (xml->getBoolAttribute(T("generated"), false))
      arguments += T(", true");
    
    writeLine(T("addMemberVariable(context, ") + arguments + T(");"));
  }

  void generateFunctionRegistrationCode(const String& className, XmlElement* xml)
  {
    String lang = xml->getStringAttribute(T("lang"), T("???"));
    String name = xml->getStringAttribute(T("name"), T("???"));
    String shortName = xml->getStringAttribute(T("shortName"), String::empty);
    String description = xml->getStringAttribute(T("description"), String::empty);

    if (lang != T("lua"))
      std::cerr << "Unsupported language " << (const char* )lang << " for function " << (const char* )name << std::endl;

    String arguments = name + T("Forwarder, T(") + name.quoted() + T(")");
    arguments += T(", ");
    arguments += shortName.isEmpty() ? T("lbcpp::string::empty") : T("T(") + shortName.quoted() + T(")");
    arguments += T(", ");
    arguments += description.isEmpty() ? T("lbcpp::string::empty") : T("T(") + description.quoted() + T(")");
    if (xml->getBoolAttribute(T("static"), false))
      arguments += T(", true");

    writeLine(T("addMemberFunction(context, ") + arguments + T(");"));
  }

  void generateClassConstructorMethod(XmlElement* xml, const String& className, const String& baseClassName)
  {
    String arguments = xml->getStringAttribute(T("arguments"), String::empty);
    String parameters = xml->getStringAttribute(T("parameters"), String::empty);
    String returnType = xml->getStringAttribute(T("returnType"), String::empty);
    
    if (returnType.isEmpty())
      returnType = baseClassName;

    StringArray tokens;
    // remove template specifiers
    String argumentsNoTemplates = arguments;
    while (argumentsNoTemplates.lastIndexOfChar('<') > -1)
    {
      int from = argumentsNoTemplates.lastIndexOfChar('<');
      int to = argumentsNoTemplates.indexOfChar(from+1, '>');
      argumentsNoTemplates = argumentsNoTemplates.replaceSection(from, to-from, T(""));
    }
    tokens.addTokens(argumentsNoTemplates, T(","), NULL);
    String argNames;
    for (int i = 0; i < tokens.size(); ++i)
    {
      String argName = tokens[i];
      int n = argName.lastIndexOfChar(' ');
      if (n >= 0)
        argName = argName.substring(n + 1);
      if (argNames.isNotEmpty())
        argNames += T(", ");
      argNames += argName;
    }

    // class declarator
    String classNameWithFirstLowerCase = replaceFirstLettersByLowerCase(className);
    String returnTypePtr = typeToRefCountedPointerType(returnType);
    openScope(returnTypePtr + T(" ") + classNameWithFirstLowerCase + T("(") + arguments + T(")"));
      writeLine(returnTypePtr + T(" res = new ") + className + T("(") + argNames + T(");"));
      String code = T("res->setThisClass(") + classNameWithFirstLowerCase + T("Class");
      if (parameters.isNotEmpty())
        code += T("(") + parameters + T(")");
      writeLine(code + T(");"));
      writeLine(T("return res;"));
    closeScope();
    newLine();
  }

  static String getMetaClass(const String& classBaseClass)
  {
    if (classBaseClass == T("Enumeration"))
      return T("Enumeration");
    else
      return T("Class");
  }

  static String getDefaultBaseType(const String& metaClass) 
    {if (metaClass == T("Enumeration")) return T("EnumValue"); else return T("Object");}


  /*
  ** Template
  */
  void generateTemplateClassDeclaration(XmlElement* xml)
  {
    String className = xml->getStringAttribute(T("name"), T("???"));
    String classBaseClass = xml->getStringAttribute(T("metaclass"), T("DefaultClass"));
    String metaClass = getMetaClass(classBaseClass);
    String baseClassName = xmlTypeToCppType(xml->getStringAttribute(T("base"), getDefaultBaseType(metaClass)));

    Declaration declaration = Declaration::makeTemplateClass(currentNamespace, className, T("Template") + metaClass);
    declarations.push_back(declaration);

    openClass(declaration.implementationClassName, T("DefaultTemplateClass"));

    // constructor
    openScope(declaration.implementationClassName + T("() : DefaultTemplateClass(T(") + className.quoted() + T("), T(") + baseClassName.quoted() + T("))"));
    closeScope();
    newLine();

    // initialize()
    openScope(T("virtual bool initialize(ExecutionContext& context)"));
      std::vector<XmlElement* > parameters;
      forEachXmlChildElementWithTagName(*xml, elt, T("parameter"))
      {
        generateParameterDeclarationInConstructor(className, elt);
        parameters.push_back(elt);
      }
      writeLine(T("return DefaultTemplateClass::initialize(context);"));
    closeScope();
    newLine();

    // instantiate
    openScope(T("virtual ClassPtr instantiate(ExecutionContext& context, const std::vector<ClassPtr>& arguments, ClassPtr baseType) const"));
      generateTemplateInstantiationFunction(xml);
    closeScope();
    newLine();

    closeClass();

    // class declarator
    String classNameWithFirstLowerCase = replaceFirstLettersByLowerCase(className) + metaClass;
    if (parameters.size() == 0)
      std::cerr << "Error: No parameters in template. Type = " << (const char *)className << std::endl;
    else
    {
      String arguments;
      String initialization;
      for (size_t i = 0; i < parameters.size(); ++i)
      {
        arguments += T("ClassPtr type") + String((int)i + 1);
        initialization += T("types[") + String((int)i) + T("] = type") + String((int)i + 1) + T("; ");

        if (i < parameters.size() - 1)
          arguments += T(", ");
      }
      writeShortFunction(metaClass + T("Ptr ") + classNameWithFirstLowerCase + T("(") + arguments + T(")"),
        T("std::vector<ClassPtr> types(") + String((int)parameters.size()) + T("); ") + initialization + T("return lbcpp::getType(T(") + className.quoted() + T("), types);")); 
    }

    // class constructors
    forEachXmlChildElementWithTagName(*xml, elt, T("constructor"))
      generateClassConstructorMethod(elt, className, baseClassName);
  }

  void generateParameterDeclarationInConstructor(const String& className, XmlElement* xml)
  {
    String type = xmlTypeToCppType(xml->getStringAttribute(T("type"), T("Object")));
    String name = xml->getStringAttribute(T("name"), T("???"));
    writeLine(T("addParameter(context, T(") + name.quoted() + T("), T(") + type.quoted() + T("));"));
  }

  void generateTemplateInstantiationFunction(XmlElement* xml)
  {
    String templateClassName = xml->getStringAttribute(T("name"), T("???"));
    String classBaseClass = xml->getStringAttribute(T("metaclass"), T("DefaultClass"));
    String metaClass = getMetaClass(classBaseClass);

    forEachXmlChildElementWithTagName(*xml, elt, T("class"))
    {
      String className = elt->getStringAttribute(T("name"));
      if (className.isEmpty())
        className = templateClassName;
      String condition = generateSpecializationCondition(elt);
      if (condition.isNotEmpty())
        writeLine(T("if (") + condition + T(")"));
      writeLine(T("return new ") + className + metaClass + T("(refCountedPointerFromThis(this), arguments, baseType);"), condition.isEmpty() ? 0 : 1);
    }
  }

  String generateSpecializationCondition(XmlElement* xml)
  {
    String res;
    forEachXmlChildElementWithTagName(*xml, elt, T("specialization"))
    {
      if (res.isNotEmpty())
        res += " && ";
      res += "inheritsFrom(context, arguments, " + elt->getStringAttribute(T("name"), T("???")).quoted() + T(", ") + elt->getStringAttribute(T("type"), T("???")).quoted() + T(")");
    }
    return res;
  }


  /*
  ** Code
  */
  void generateCode(XmlElement* elt)
  {
    StringArray lines;
    lines.addTokens(elt->getAllSubText(), T("\n"), NULL);
    int minimumSpaces = 0x7FFFFFFF;
    for (int i = 0; i < lines.size(); ++i)
    {
      String line = lines[i];
      int numSpaces = line.length() - line.trimStart().length();
      if (numSpaces && line.substring(numSpaces).trim().isNotEmpty())
        minimumSpaces = jmin(numSpaces, minimumSpaces);
    }

    int spaces = minimumSpaces == 0x7FFFFFFF ? 0 : minimumSpaces;
    for (int i = 0; i < lines.size(); ++i)
    {
      String line = lines[i];
      int numSpaces = line.length() - line.trimStart().length();
      writeLine(lines[i].substring(jmin(numSpaces, spaces)));
    }
  }

  /*
  ** Footer
  */
  void generateLibraryClass()
  {
    String variableName = replaceFirstLettersByLowerCase(fileName) + T("Library");

    forEachXmlChildElementWithTagName(*xml, elt, T("import"))
    {
      String ifdef = elt->getStringAttribute(T("ifdef"));
      if (ifdef.isNotEmpty())
        writeLine(T("#ifdef ") + ifdef);
      String name = elt->getStringAttribute(T("name"), T("???"));
      name = replaceFirstLettersByLowerCase(name) + T("Library");
      writeLine(T("extern lbcpp::LibraryPtr ") + name + T("();"));
      writeLine(T("extern void ") + name + T("CacheTypes(ExecutionContext& context);"));
      writeLine(T("extern void ") + name + T("UnCacheTypes();"));
      if (ifdef.isNotEmpty())
      {
        writeLine(T("#else // ") + ifdef);
        writeLine(T("inline lbcpp::LibraryPtr ") + name + T("() {return lbcpp::LibraryPtr();}"));
        writeLine(T("inline void ") + name + T("CacheTypes(ExecutionContext& context) {}"));
        writeLine(T("inline void ") + name + T("UnCacheTypes() {}"));
        writeLine(T("#endif // ") + ifdef);
      }
    }

    // cacheTypes function
    openScope(T("void ") + variableName + T("CacheTypes(ExecutionContext& context)"));
    forEachXmlChildElementWithTagName(*xml, elt, T("import"))
    {
      String name = elt->getStringAttribute(T("name"), T("???"));
      writeLine(replaceFirstLettersByLowerCase(name) + T("LibraryCacheTypes(context);"));
    }
    for (size_t i = 0; i < declarations.size(); ++i)
    {
      const Declaration& declaration = declarations[i];
      if (declaration.cacheVariableName.isNotEmpty())
        writeLine(declaration.getCacheVariableFullName() + T(" = typeManager().getType(context, T(") + declaration.getFullName().quoted() + T("));"));
    }
    closeScope();
    
    // unCacheTypes function
    openScope(T("void ") + variableName + T("UnCacheTypes()"));
    forEachXmlChildElementWithTagName(*xml, elt, T("import"))
    {
      String name = elt->getStringAttribute(T("name"), T("???"));
      writeLine(replaceFirstLettersByLowerCase(name) + T("LibraryUnCacheTypes();"));
    }
    for (size_t i = 0; i < declarations.size(); ++i)
    {
      const Declaration& declaration = declarations[i];
      if (declaration.cacheVariableName.isNotEmpty())
        writeLine(declaration.getCacheVariableFullName() + T(".clear();"));
    }
    closeScope();

    // Library class
    openClass(fileName + T("Library"), T("Library"));
    
    // constructor
    openScope(fileName + T("Library() : Library(T(") + fileName.quoted() + T("))"));
    closeScope();
    newLine();

    // initialize function
    openScope(T("virtual bool initialize(ExecutionContext& context)"));
    writeLine(T("bool __ok__ = true;"));

    for (size_t i = 0; i < declarations.size(); ++i)
    {
      const Declaration& declaration = declarations[i];
      if (declaration.type == Declaration::uiComponentDeclaration)
      {
        writeLine(T("__ok__ &= declareUIComponent(context, T(") + declaration.name.quoted() +
          T("), MakeUIComponentConstructor< ") + declaration.implementationClassName + T(">::ctor);"));
      }
      else
      {
        String code = T("__ok__ &= declare");
        if (declaration.type == Declaration::templateTypeDeclaration)
          code += T("TemplateClass");
        else if (declaration.type == Declaration::typeDeclaration)
          code += T("Type");
        code += T("(context, new ") + declaration.getImplementationClassFullName() + T(");");
        writeLine(code);
      }
    }

    forEachXmlChildElementWithTagName(*xml, elt, T("import"))
    {
      String name = elt->getStringAttribute(T("name"), T("???"));
      writeLine(T("__ok__ &= declareSubLibrary(context, ") + replaceFirstLettersByLowerCase(name) + T("Library());"));
    }

    writeLine(T("return __ok__;"));
    closeScope();
    
    newLine();
    writeShortFunction(T("virtual void cacheTypes(ExecutionContext& context)"), variableName + T("CacheTypes(context);"));
    writeShortFunction(T("virtual void uncacheTypes()"), variableName + T("UnCacheTypes();"));

    closeClass();
    
    writeShortFunction(T("lbcpp::LibraryPtr ") + variableName + T("()"), T("return new ") + fileName + T("Library();"));
  }

  void generateDynamicLibraryFunctions()
  {
    openScope(T("extern \"C\""));

    newLine();
    writeLine(T("# ifdef WIN32"));
    writeLine(T("#  define OIL_EXPORT  __declspec( dllexport )"));
    writeLine(T("# else"));
    writeLine(T("#  define OIL_EXPORT"));
    writeLine(T("# endif"));
    newLine();

    openScope(T("OIL_EXPORT Library* lbcppInitializeLibrary(lbcpp::ApplicationContext& applicationContext)"));
    writeLine(T("lbcpp::initializeDynamicLibrary(applicationContext);"));
    writeLine(T("LibraryPtr res = ") + replaceFirstLettersByLowerCase(fileName) + T("Library();"));
    writeLine(T("res->incrementReferenceCounter();"));
    writeLine(T("return res.get();"));
    closeScope();

    openScope(T("OIL_EXPORT void lbcppDeinitializeLibrary()"));
    writeLine(replaceFirstLettersByLowerCase(fileName) + T("LibraryUnCacheTypes();"));
    writeLine(T("lbcpp::deinitializeDynamicLibrary();"));
    closeScope();

    closeScope(); // extern "C"
  }

private:
  XmlElement* xml;
  OutputStream& ostr;
  String fileName;
  String directoryName;
  int indentation;
  String currentNamespace;

  struct Declaration
  {
    enum Type
    {
      typeDeclaration,
      templateTypeDeclaration,
      uiComponentDeclaration,
    } type;

    static Declaration makeType(const String& namespaceName, const String& typeName, const String& kind)
    {
      Declaration res;
      res.namespaceName = namespaceName;
      res.type = typeDeclaration;
      res.name = typeName;
      res.implementationClassName = typeName + kind;
      res.cacheVariableName = replaceFirstLettersByLowerCase(typeName) + kind;
      return res;
    }

    static Declaration makeTemplateClass(const String& namespaceName, const String& typeName, const String& kind)
    {
      Declaration res;
      res.namespaceName = namespaceName;
      res.type = templateTypeDeclaration;
      res.name = typeName;
      res.implementationClassName = typeName + kind;
      return res;
    }

    static Declaration makeUIComponent(const String& namespaceName, const String& className, const String& typeName)
    {
      Declaration res;
      res.namespaceName = namespaceName;
      res.type = uiComponentDeclaration;
      res.implementationClassName = className;
      res.name = typeName;
      return res;
    }

    String name;
    String namespaceName;
    String implementationClassName;
    String cacheVariableName;

    String getFullName() const // namespace and name
      {return namespaceName.isEmpty() ? name : namespaceName + T("::") + name;}

    String getCacheVariableFullName() const
      {return namespaceName.isEmpty() ? cacheVariableName : namespaceName + T("::") + cacheVariableName;}

    String getImplementationClassFullName() const
      {return namespaceName.isEmpty() ? implementationClassName : namespaceName + T("::") + implementationClassName;}
  };

  std::vector<Declaration> declarations;

  void newLine(int indentationOffset = 0)
  {
    ostr << "\n";
    for (int i = 0; i < jmax(0, indentation + indentationOffset); ++i)
      ostr << "  ";
  }

  void writeLine(const String& line, int indentationOffset = 0)
    {newLine(indentationOffset); ostr << line;}

  void openScope(const String& declaration)
  {
    newLine();
    ostr << declaration;
    newLine();
    ostr << "{";
    ++indentation;
  }

  void closeScope(const String& closingText = String::empty)
  {
    jassert(indentation > 0);
    --indentation;
    newLine();
    ostr << "}" << closingText;
  }

  void openClass(const String& className, const String& baseClass)
  {
    openScope(T("class ") + className + T(" : public ") + baseClass);
    writeLine(T("public:"), -1);
  }

  void closeClass()
    {newLine(); writeLine(T("lbcpp_UseDebuggingNewOperator")); closeScope(T(";")); newLine();}

  void writeShortFunction(const String& declaration, const String& oneLineBody)
  {
    writeLine(declaration);
    writeLine(T("{") + oneLineBody + T("}"), 1);
    newLine();
  }
};

class XmlMacros
{
public:
  void registerMacrosRecursively(const XmlElement* xml)
  {
    if (xml->getTagName() == T("defmacro"))
      m[xml->getStringAttribute(T("name"))] = xml;
    else
      for (int i = 0; i < xml->getNumChildElements(); ++i)
        registerMacrosRecursively(xml->getChildElement(i));
  }

  typedef std::map<String, String> variables_t;

  String processText(const String& str, const variables_t& variables)
  {
    String res = str;
    for (std::map<String, String>::const_iterator it = variables.begin(); it != variables.end(); ++it)
      res = res.replace(T("%{") +  it->first + T("}"), it->second);
    return res;
  }

  // returns a new XmlElement
  XmlElement* process(const XmlElement* xml, const variables_t& variables = variables_t())
  {
    if (xml->getTagName() == T("defmacro"))
      return NULL;

    jassert(xml->getTagName() != T("macro"));

    if (xml->isTextElement())
      return XmlElement::createTextElement(processText(xml->getText(), variables));

    XmlElement* res = new XmlElement(xml->getTagName());
    for (int i = 0; i < xml->getNumAttributes(); ++i)
      res->setAttribute(xml->getAttributeName(i), processText(xml->getAttributeValue(i), variables));

    for (int i = 0; i < xml->getNumChildElements(); ++i)
    {
      XmlElement* child = xml->getChildElement(i);
      if (child->getTagName() == T("macro"))
      {
        String name = processText(child->getStringAttribute(T("name")), variables);
        macros_t::iterator it = m.find(name);
        if (it == m.end())
        {
          std::cerr << "Could not find macro " << (const char* )name << std::endl;
          return NULL;
        }
        else
        {
          variables_t nvariables(variables);
          for (int j = 0; j < child->getNumAttributes(); ++j)
            if (child->getAttributeName(j) != T("name"))
              nvariables[child->getAttributeName(j)] = child->getAttributeValue(j);
          const XmlElement* macro = it->second;
          for (int j = 0; j < macro->getNumChildElements(); ++j)
            addChild(res, process(macro->getChildElement(j), nvariables));
        }
      }
      else
        addChild(res, process(child, variables));
    }
    return res;
  }

private:
  typedef std::map<String, const XmlElement* > macros_t;

  void addChild(XmlElement* xml, XmlElement* child)
  {
    if (xml && child)
      xml->addChildElement(child);
  }

  macros_t m;
};

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " input.xml output.cpp" << std::endl;
    return 1;
  }

  SystemStats::initialiseStats();

  File output(argv[2]);
  output.deleteFile();
  OutputStream* ostr = File(argv[2]).createOutputStream();

  inputFile = File(argv[1]);
  
  XmlDocument xmldoc(inputFile);
  XmlElement* iroot = xmldoc.getDocumentElement();
  String error = xmldoc.getLastParseError();
  if (error != String::empty)
  {
    std::cerr << "Parse error: " << (const char* )error << std::endl;
    if (iroot)
      delete iroot;
    return 2;
  }

  XmlMacros macros;
  macros.registerMacrosRecursively(iroot);
  XmlElement* root = macros.process(iroot);
  delete iroot;
  if (!root)
    return 2;

  CppCodeGenerator(root, *ostr).generate();

  delete root;
  delete ostr;
  return 0;
}
