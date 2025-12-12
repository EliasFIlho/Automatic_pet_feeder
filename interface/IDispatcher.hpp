#pragma once

#include "IListener.hpp"

class IDispatcher
{
public:
  virtual ~IDispatcher() {};
  virtual void Attach(IListener *listener) = 0;
  virtual void Notify(Events evt) = 0;
};