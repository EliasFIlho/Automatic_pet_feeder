#pragma once
#include "Events.hpp"

class IListener
{
public:
  virtual ~IListener() {};
  virtual void Update(Events evt) = 0;
};
