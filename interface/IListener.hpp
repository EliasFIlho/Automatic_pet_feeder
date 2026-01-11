#pragma once
#include "Enums.hpp"

class IListener
{
public:
  virtual ~IListener() {};
  virtual void Update(Events evt) = 0;
};
