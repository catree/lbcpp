/*-----------------------------------------.---------------------------------.
| Filename: PieChart.h                     | Pie Chart Object                |
| Author  : Julien Becker                  |                                 |
| Started : 08/06/2011 13:56               |                                 |
`------------------------------------------/                                 |
                               |                                             |
                               `--------------------------------------------*/

#ifndef LBCPP_USER_INTERFACE_PIE_CHART_H_
# define LBCPP_USER_INTERFACE_PIE_CHART_H_

# include <oil/UserInterface/ObjectComponent.h>

namespace lbcpp
{

class PieChart : public NameableObject
{
public:
  PieChart(const string& name = string::empty)
    : NameableObject(name) {}

  size_t getNumElements() const
    {return values.size();}

  void appendElement(const string& name, double value)
    {values.push_back(std::make_pair(name, value));}

  const string& getElementName(size_t index) const
    {jassert(index < values.size()); return values[index].first;}

  double getElementValue(size_t index) const
    {jassert(index < values.size()); return values[index].second;}

protected:
  friend class PieChartClass;

  std::vector<std::pair<string, double> > values;
};

typedef ReferenceCountedObjectPtr<PieChart> PieChartPtr;

}; /* namespace lbcpp */

#endif // !LBCPP_USER_INTERFACE_PIE_CHART_H_
