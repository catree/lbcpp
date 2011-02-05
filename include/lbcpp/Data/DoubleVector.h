/*-----------------------------------------.---------------------------------.
| Filename: DoubleVector.h                 | Base class for Double Vectors   |
| Author  : Francis Maes                   |                                 |
| Started : 03/02/2011 16:10               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_DATA_DOUBLE_VECTOR_H_
# define LBCPP_DATA_DOUBLE_VECTOR_H_

# include "../Core/Vector.h"

namespace lbcpp
{

class FeatureGenerator;
typedef ReferenceCountedObjectPtr<FeatureGenerator> FeatureGeneratorPtr;

class DoubleVector;
typedef ReferenceCountedObjectPtr<DoubleVector> DoubleVectorPtr;

class SparseDoubleVector;
typedef ReferenceCountedObjectPtr<SparseDoubleVector> SparseDoubleVectorPtr;

class DenseDoubleVector;
typedef ReferenceCountedObjectPtr<DenseDoubleVector> DenseDoubleVectorPtr;

class LazyDoubleVector;
typedef ReferenceCountedObjectPtr<LazyDoubleVector> LazyDoubleVectorPtr;

class CompositeDoubleVector;
typedef ReferenceCountedObjectPtr<CompositeDoubleVector> CompositeDoubleVectorPtr;

class DoubleVector : public Vector
{
public:
  DoubleVector(ClassPtr thisClass)
    : Vector(thisClass) {}
  DoubleVector() {}

  virtual EnumerationPtr getElementsEnumeration() const
    {return thisClass->getTemplateArgument(0);}

  virtual TypePtr getElementsType() const
    {return thisClass->getTemplateArgument(1);}

  Variable missingElement() const
    {return Variable::missingValue(getElementsType());}

  static EnumerationPtr getElementsEnumeration(TypePtr doubleVectorType);
  static bool getTemplateParameters(ExecutionContext& context, TypePtr type, EnumerationPtr& elementsEnumeration, TypePtr& elementsType);

  virtual size_t l0norm() const = 0;
  virtual void appendTo(const SparseDoubleVectorPtr& sparseVector, size_t offsetInSparseVector) const = 0;
  virtual void addWeightedTo(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector, double weight) const = 0;
  virtual double dotProduct(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector) const = 0;

  double dotProduct(const DenseDoubleVectorPtr& denseVector)
    {return dotProduct(denseVector, 0);}

  void addTo(const DenseDoubleVectorPtr& denseVector)
    {addWeightedTo(denseVector, 0, 1.0);}
  
  void subtractFrom(const DenseDoubleVectorPtr& denseVector)
    {addWeightedTo(denseVector, 0, -1.0);}
};

extern ClassPtr doubleVectorClass(TypePtr elementsEnumeration = enumValueType, TypePtr elementsType = doubleType);
extern ClassPtr sparseDoubleVectorClass(TypePtr elementsEnumeration = enumValueType, TypePtr elementsType = doubleType);
extern ClassPtr denseDoubleVectorClass(TypePtr elementsEnumeration = enumValueType, TypePtr elementsType = doubleType);
extern ClassPtr lazyDoubleVectorClass(TypePtr elementsEnumeration = enumValueType, TypePtr elementsType = doubleType);
extern ClassPtr compositeDoubleVectorClass(TypePtr elementsEnumeration = enumValueType, TypePtr elementsType = doubleType);

class SparseDoubleVector : public DoubleVector
{
public:
  SparseDoubleVector(EnumerationPtr elementsEnumeration, TypePtr elementsType);
  SparseDoubleVector(ClassPtr thisClass);
  SparseDoubleVector();

  void appendValue(size_t index, double value)
    {jassert((int)index > lastIndex); values.push_back(std::make_pair(index, value)); lastIndex = (int)index;}

  std::vector< std::pair<size_t, double> >& getValues()
    {return values;}

  int getLastIndex() const
    {return lastIndex;}

  void updateLastIndex()
    {lastIndex = values.size() ? (int)values.back().first : -1;}

  // DoubleVector
  virtual size_t l0norm() const;
  virtual void appendTo(const SparseDoubleVectorPtr& sparseVector, size_t offsetInSparseVector) const;
  virtual void addWeightedTo(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector, double weight) const;
  virtual double dotProduct(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector) const;

  // Vector
  virtual void clear();
  virtual void reserve(size_t size);
  virtual void resize(size_t size);
  virtual void prepend(const Variable& value);
  virtual void append(const Variable& value);
  virtual void remove(size_t index);

  // Container
  virtual size_t getNumElements() const;
  virtual Variable getElement(size_t index) const;
  virtual void setElement(size_t index, const Variable& value);

private:
  std::vector< std::pair<size_t, double> > values;
  int lastIndex;
};

class DenseDoubleVector : public DoubleVector
{
public:
  DenseDoubleVector(ClassPtr thisClass, std::vector<double>& values);
  DenseDoubleVector(ClassPtr thisClass, size_t initialSize = (size_t)-1, double initialValue = 0.0);
  DenseDoubleVector();
  virtual ~DenseDoubleVector();
  
  std::vector<double>& getValues()
    {jassert(values); return *values;}

  double* getValuePointer(size_t index)
    {jassert(values && index <= values->size()); return &(*values)[0] + index;}

  const double* getValuePointer(size_t index) const
    {jassert(values && index <= values->size()); return &(*values)[0] + index;}

  double& getValueReference(size_t index)
    {return *getValuePointer(index);}

  double getValue(size_t index) const
    {jassert(values); return index < values->size() ? (*values)[index] : 0.0;}

  void incrementValue(size_t index, double value)
    {jassert(values && index < values->size()); (*values)[index] += value;}

  void ensureSize(size_t minimumSize);

  void multiplyByScalar(double value);

  // DoubleVector
  virtual size_t l0norm() const;
  virtual void appendTo(const SparseDoubleVectorPtr& sparseVector, size_t offsetInSparseVector) const;
  virtual void addWeightedTo(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector, double weight) const;
  virtual double dotProduct(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector) const;

  // Vector
  virtual void clear();
  virtual void reserve(size_t size);
  virtual void resize(size_t size);
  virtual void prepend(const Variable& value);
  virtual void append(const Variable& value);
  virtual void remove(size_t index);

  // Container
  virtual size_t getNumElements() const;
  virtual Variable getElement(size_t index) const;
  virtual void setElement(size_t index, const Variable& value);

  // Object
  virtual void clone(ExecutionContext& context, const ObjectPtr& target) const;

private:
  std::vector<double>* values;
  bool ownValues;
};

class LazyDoubleVector : public DoubleVector
{
public:
  LazyDoubleVector(FeatureGeneratorPtr featureGenerator, const Variable* inputs);
  LazyDoubleVector() {}

  // DoubleVector
  virtual size_t l0norm() const;
  virtual void appendTo(const SparseDoubleVectorPtr& sparseVector, size_t offsetInSparseVector) const;
  virtual void addWeightedTo(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector, double weight) const;
  virtual double dotProduct(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector) const;

  // Vector
  virtual void clear();
  virtual void reserve(size_t size);
  virtual void resize(size_t size);
  virtual void prepend(const Variable& value);
  virtual void append(const Variable& value);
  virtual void remove(size_t index);

  // Container
  virtual size_t getNumElements() const;
  virtual Variable getElement(size_t index) const;
  virtual void setElement(size_t index, const Variable& value);

private:
  friend class LazyDoubleVectorClass;

  FeatureGeneratorPtr featureGenerator;
  std::vector<Variable> inputs;
  DoubleVectorPtr computedVector;

  void ensureIsComputed();
};

class CompositeDoubleVector : public DoubleVector
{
public:
  CompositeDoubleVector(ClassPtr thisClass)
    : DoubleVector(thisClass) {}
  CompositeDoubleVector() {}
  
  // DoubleVector
  virtual size_t l0norm() const;
  virtual void appendTo(const SparseDoubleVectorPtr& sparseVector, size_t offsetInSparseVector) const;
  virtual void addWeightedTo(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector, double weight) const;
  virtual double dotProduct(const DenseDoubleVectorPtr& denseVector, size_t offsetInDenseVector) const;

  // Vector
  virtual void clear();
  virtual void reserve(size_t size);
  virtual void resize(size_t size);
  virtual void prepend(const Variable& value);
  virtual void append(const Variable& value);
  virtual void remove(size_t index);

  // Container
  virtual size_t getNumElements() const;
  virtual Variable getElement(size_t index) const;
  virtual void setElement(size_t index, const Variable& value);

  void appendSubVector(size_t shift, const DoubleVectorPtr& subVector);

private:
  std::vector< std::pair<size_t, DoubleVectorPtr> > vectors;
};

}; /* namespace lbcpp */

#endif // !LBCPP_DATA_DOUBLE_VECTOR_H_