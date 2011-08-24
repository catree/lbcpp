-- Cound Distinct Formulas Script

require '../ExpressionDiscovery/ReversePolishNotationProblem'
require 'Random'
require 'Stochastic'

----------------
--require 'AST'
--require 'Language.LuaChunk'
--ast = AST.parseExpressionFromString("(x * x * x + y * y * y + z * z * z - 3 * x * y * z) / (x + y + z)")
--print (ast:print())
--print (ast:simplify():print())
----------------

problem = DecisionProblem.ReversePolishNotation{
  variables = {"a", "b", "c", "d"},
  constants = {1, 2, 5, 10},
  unaryOperations = {"unm"},
  binaryOperations = {"add", "sub", "mul", "div"},
  objective = | | 0,
  maxSize = 4
}

local dataset = {}
for i = 1,10 do
  local example = {
    Stochastic.standardGaussian(), 
    Stochastic.standardGaussian(), 
    Stochastic.standardGaussian(), 
    Stochastic.standardGaussian()
  }
  table.insert(dataset, example)
end

function makeUniqueKey(formula)
  local f = buildExpression(problem.__parameters.variables, formula)
  local res = ""
  for i,example in ipairs(dataset) do
    local y = f(unpack(example))
    assert(not(y == nil))
    --print (y)
    if y > -math.huge and y < math.huge then
      y = math.floor(y * 10000 + 0.5)
    else
      y = math.huge
    end
    res = res .. ";" .. y
  end
  return res
end

local numFinalStates = 0
local numDistinctFinalStates = 0
local numDistinctSimplifiedFinalStates = 0
local numUniqueFinalStates = 0
local finalStates = {}
local simplifiedFinalStates = {}
local uniqueFinalStates = {}

local function countDistinctFormulas(problem, x)
  if problem.isFinal(x) then

    -- num final states
    numFinalStates = numFinalStates + 1

    -- num distinct final states
    local str1 = problem.stateToString(x)
    if finalStates[str1] == nil then
      finalStates[str1] = true
      numDistinctFinalStates = numDistinctFinalStates + 1
    end

    -- num distinct simplified final states
    local str2 = problem.stateToString(x:simplify())
    if simplifiedFinalStates[str2] == nil then
      simplifiedFinalStates[str2] = true
      numDistinctSimplifiedFinalStates = numDistinctSimplifiedFinalStates + 1
      --print (str1, "-->", str2)
    end

    -- num unique formulas
    local str3 = makeUniqueKey(x)
    if uniqueFinalStates[str3] == nil then
      uniqueFinalStates[str3] = {}
      numUniqueFinalStates = numUniqueFinalStates + 1
    end
    uniqueFinalStates[str3][str2] = true

    if not (str3 == makeUniqueKey(x:simplify())) then
      print (str1, str2, str3, makeUniqueKey(x:simplify()))
    end
    assert(str3 == makeUniqueKey(x:simplify()))
    --print ("=>", str3)--str1, str2, str3)

  else
    local U = problem.U(x)
    for i,u in ipairs(U) do
      countDistinctFormulas(problem, problem.f(x, u))
    end
  end
end

countDistinctFormulas(problem, problem.x0)
print ("NumFinalStates", numFinalStates)
print ("NumDistinctFinalStates", numDistinctFinalStates)
print ("NumDistinctSimplifiedFinalStates", numDistinctSimplifiedFinalStates)
print ("NumUniqueFinalStates", numUniqueFinalStates)

for key,tbl in pairs(uniqueFinalStates) do
  local content = {}
  for str,b in pairs(tbl) do table.insert(content, str) end
  if #content > 1 then
    print (table.concat(content, " ; "))
  end
end