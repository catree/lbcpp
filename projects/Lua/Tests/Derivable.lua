local function twiceWithLocalVariables(derivable x)
  local y = 2 * x
  for i = 1,5 do
    if y > 0 then
      y = sigmoid(y)
    end
  end
  return y
end

local function perceptronLoss(derivable theta, derivable input, y)
  local activation = dotProduct(input, theta)
  local sign = (y and 1 or -1)
  return hingeLoss(activation * sign)
end

local function chainOfIdentity(derivable x)
  local a = x
  local b = a
  local c = b
  local d = c
  return d
end

local function twice(derivable x)
  return 2 * x
end

local function twice_id(derivable x)
  return twice(x)
end

local function constant(derivable x)
  return 51
end

local function identity(derivable x)
  return x
end

local function unaryMinus(derivable x)
  return -x
end

local function add(derivable x, derivable y)
  return x + y
end

local function twice2d(derivable x, derivable y)
  return 2 * (x * y)
end

local function twice2dbis(derivable x, y)
  return 2 * (x * y)
end

-- square loss function
local function squareLoss(derivable ycorrect, derivable ypred)
  return (ypred - ycorrect)^2
end

-- exponential loss function
local function expLoss(derivable x)
  return math.exp(-x)
end

-- Perceptron loss function
local function perceptronLoss(derivable x)
  return math.max(-x, 0.0)
end

-- LargeMargin loss function
local function largeMarginLoss(derivable x)
  return math.max(1 - x, 0.0)
end

-- LogBinomial loss function
local function logBinomialLoss(derivable x)
  if x < -10 then -- avoid approximation errors in the exp(-x) formula
    return -x
  else
    return math.log(1 + math.exp(-x))
  end
end


-- Sigmoid
local function sigmoid(derivable x)
  return 1 / (1 + math.exp(-x))
end

-- a function that should not be transformed
local function nothingToSeeHere(x, y)
  return 2 * x * y
end
