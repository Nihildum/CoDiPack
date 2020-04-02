#pragma once

#include <stdio.h>

#include "../testInterface.hpp"

enum class DriverOrder {
  Primal,
  Deriv1st,
  Deriv2nd
};

template<typename Number>
struct DriverInterface {
  public:

    virtual ~DriverInterface() {}

    virtual std::string getName() = 0;
    virtual DriverOrder getOrder() = 0;
    virtual TestVector<Number> getTests() = 0;

    virtual void runTest(TestInterface<Number>* test, FILE* out) = 0;
};
