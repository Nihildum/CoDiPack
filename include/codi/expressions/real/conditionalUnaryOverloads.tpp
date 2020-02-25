/*
 * In order to include this file the user has to define the preprocessor macro OPERATOR.
 * OPERATOR contains the name of the comparison operator without the 'operator' classifier.
 * e.g. '!'.
 *
 * The define OPERATOR will be undefined at the end of this template.
 */

#ifndef OPERATOR
  #error Please define the name of the operator.
#endif

// Create argA correct include environment for viewing and programming in an IDE
#ifndef OPERATOR
  #define OPERATOR !
  #include "../../aux/macros.h"
  #include "../../config.h"
  #include "../expressionInterface.hpp"

  namespace codi {
#endif

  template<typename Real, typename Arg>
  CODI_INLINE bool operator OPERATOR(ExpressionInterface<Real, ArgA> const& arg) {
    return OPERATOR getPassiveValue(argA);
  }

// Create argA correct include environment for viewing and programming in an IDE
#ifndef OPERATOR
  }
#endif

#undef OPERATOR