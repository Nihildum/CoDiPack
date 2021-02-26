#pragma once

#include <algorithm>
#include <functional>
#include <type_traits>

#include "../../aux/macros.hpp"
#include "../../expressions/activeType.hpp"
#include "../../traits/expressionTraits.hpp"
#include "directStatementEvaluator.hpp"
#include "statementEvaluatorInterface.hpp"

/** \copydoc codi::Namespace */
namespace codi {

  /**
   * @brief Additional data required by an InnerStatementEvaluator.
   */
  struct InnerPrimalTapeStatementData : public PrimalTapeStatementFunctions {
    public:

      using Base = PrimalTapeStatementFunctions;  ///< Base class abbreviation

      size_t maxActiveArguments;  ///< Maximum number of active arguments.
      size_t maxConstantArguments;  ///< Maximum number of constant arguments.

      /// Constructor
      InnerPrimalTapeStatementData(size_t maxActiveArguments, size_t maxConstantArguments,
                                   typename Base::Handle forward, typename Base::Handle primal,
                                   typename Base::Handle reverse)
          : Base(forward, primal, reverse),
            maxActiveArguments(maxActiveArguments),
            maxConstantArguments(maxConstantArguments) {}
  };

  /// Store InnerPrimalTapeStatementData as static variables.
  template<typename Tape, typename Expr>
  struct InnerStatementEvaluatorStaticStore {
    public:

      static InnerPrimalTapeStatementData const staticStore;  ///< Static storage.
  };

  template<typename Generator, typename Expr>
  InnerPrimalTapeStatementData const InnerStatementEvaluatorStaticStore<Generator, Expr>::staticStore(
      ExpressionTraits::NumberOfActiveTypeArguments<Expr>::value,
      ExpressionTraits::NumberOfConstantTypeArguments<Expr>::value,
      (typename PrimalTapeStatementFunctions::Handle)Generator::template statementEvaluateForwardInner<Expr>,
      (typename PrimalTapeStatementFunctions::Handle)Generator::template statementEvaluatePrimalInner<Expr>,
      (typename PrimalTapeStatementFunctions::Handle)Generator::template statementEvaluateReverseInner<Expr>);

  /**
   * @brief Expression evaluation in the inner function. Data loading in the compilation context of the tape.
   * Storing in static context.
   *
   * See StatementEvaluatorInterface for details.
   *
   * @tparam _Real  The computation type of a tape usually defined by ActiveType::Real.
   */
  template<typename _Real>
  struct InnerStatementEvaluator : public StatementEvaluatorInterface<_Real> {
    public:

      using Real = CODI_DD(_Real, double);  ///< See InnerStatementEvaluator

      /*******************************************************************************/
      /// @name StatementEvaluatorInterface implementation
      /// @{

      using Handle = InnerPrimalTapeStatementData const*;  ///< Pointer to static storage location.

      /// \copydoc StatementEvaluatorInterface::callForward
      template<typename Tape, typename... Args>
      static Real callForward(Handle const& h, Args&&... args) {
        return Tape::statementEvaluateForwardFull((FunctionForward<Tape>)h->forward, h->maxActiveArguments,
                                                  h->maxConstantArguments, std::forward<Args>(args)...);
      }

      /// \copydoc StatementEvaluatorInterface::callPrimal
      template<typename Tape, typename... Args>
      static Real callPrimal(Handle const& h, Args&&... args) {
        return Tape::statementEvaluatePrimalFull((FunctionPrimal<Tape>)h->primal, h->maxActiveArguments,
                                                 h->maxConstantArguments, std::forward<Args>(args)...);
      }

      /// \copydoc StatementEvaluatorInterface::callReverse
      template<typename Tape, typename... Args>
      static void callReverse(Handle const& h, Args&&... args) {
        Tape::statementEvaluateReverseFull((FunctionReverse<Tape>)h->reverse, h->maxActiveArguments,
                                           h->maxConstantArguments, std::forward<Args>(args)...);
      }

      /// \copydoc StatementEvaluatorInterface::createHandle
      template<typename Tape, typename Generator, typename Expr>
      static Handle createHandle() {
        return &InnerStatementEvaluatorStaticStore<Generator, Expr>::staticStore;
      }

    /// @}

    protected:

      /// Full forward function type.
      template<typename Tape>
      using FunctionForward = decltype(&Tape::template statementEvaluateForwardInner<ActiveType<Tape>>);

      /// Full primal function type.
      template<typename Tape>
      using FunctionPrimal = decltype(&Tape::template statementEvaluatePrimalInner<ActiveType<Tape>>);

      /// Full reverse function type.
      template<typename Tape>
      using FunctionReverse = decltype(&Tape::template statementEvaluateReverseInner<ActiveType<Tape>>);
  };
}
