#pragma once

#include "../../aux/macros.hpp"
#include "../../config.h"

#include "customAdjointVectorEvaluationTapeInterface.hpp"
#include "dataManagementTapeInterface.hpp"
#include "externalFunctionTapeInterface.hpp"
#include "forwardEvaluationTapeInterface.hpp"
#include "gradientAccessTapeInterface.hpp"
#include "identifierInformationTapeInterface.hpp"
#include "internalStatementRecordingInterface.hpp"
#include "manualStatementPushTapeInterface.hpp"
#include "positionalEvaluationTapeInterface.hpp"
#include "preaccumulationEvaluationTapeInterface.hpp"
#include "primalEvaluationTapeInterface.hpp"
#include "reverseTapeInterface.hpp"

/** \copydoc codi::Namespace */
namespace codi {

  /**
   * @brief Full tape interface definitions, that supports all features of CoDiPack.
   *
   * See \ref TapeInterfaces for a general overview of the tape interface design in CoDiPack.
   *
   * A tape that implements this interface correctly, can be used in all helper structures of CoDiPack.
   *
   * @tparam _Real        The computation type of a tape usually defined by ActiveType::Real.
   * @tparam _Gradient    The gradient type of a tape usually defined by ActiveType::Gradient.
   * @tparam _Identifier  The adjoint/tangent identification of a tape usually defined by ActiveType::Identifier.
   * @tparam _Position  Global tape position usually defined by Tape::Position.
   */
  template<typename _Real, typename _Gradient, typename _Identifier, typename _Position>
  struct FullTapeInterface :
      public virtual CustomAdjointVectorEvaluationTapeInterface<_Position>,
      public virtual DataManagementTapeInterface,
      public virtual ExternalFunctionTapeInterface<_Real, _Gradient, _Identifier>,
      public virtual ForwardEvaluationTapeInterface<_Position>,
      public virtual GradientAccessTapeInterface<_Gradient, _Identifier>,
      public virtual IdentifierInformationTapeInterface<_Real, _Gradient, _Identifier>,
      public virtual InternalStatementRecordingInterface<_Identifier>,
      public virtual ManualStatementPushTapeInterface<_Real, _Gradient, _Identifier>,
      public virtual PositionalEvaluationTapeInterface<_Position>,
      public virtual PreaccumulationEvaluationTapeInterface<_Real, _Gradient, _Identifier, _Position>,
      public virtual PrimalEvaluationTapeInterface<_Real, _Identifier, _Position>,
      public virtual ReverseTapeInterface<_Real, _Gradient, _Identifier>
  {
    public:

      using Real = CODI_DECLARE_DEFAULT(_Real, double); ///< See FullTapeInterface
      using Gradient = CODI_DECLARE_DEFAULT(_Gradient, double); ///< See FullTapeInterface
      using Identifier = CODI_DECLARE_DEFAULT(_Identifier, int); ///< See FullTapeInterface
      using Position = CODI_DECLARE_DEFAULT(_Position, EmptyPosition); ///< See FullTapeInterface

  };
}
