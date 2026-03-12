#pragma once

#include "IListener.hpp"

class IDispatcher
{
public:
  virtual ~IDispatcher() {};
  virtual void Attach(IListener *listener, uint8_t evt_group_maks) = 0;
  virtual void Notify(Events evt, uint8_t evt_group_maks) = 0;
};