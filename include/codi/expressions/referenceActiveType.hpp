#pragma once

#include "../aux/macros.hpp"
#include "../config.h"
#include "../tapes/interfaces/gradientAccessTapeInterface.hpp"
#include "../traits/realTraits.hpp"
#include "assignmentOperators.hpp"
#include "incrementOperators.hpp"
#include "lhsExpressionInterface.hpp"

/** \copydoc codi::Namespace */
namespace codi {

  /**
   * @brief Holds a reference to an ActiveType for manual optimization of common arguments.
   *
   * See the tutorial TODO. For an example use and explanation.
   *
   * @tparam _Type  The type of the reference which is captured.
   */
  template<typename _Type>
  struct ReferenceActiveType : public LhsExpressionInterface<typename _Type::Real, typename _Type::Gradient, typename _Type::Tape, ReferenceActiveType<_Type> >,
                               public AssignmentOperators<_Type, ReferenceActiveType<_Type>>,
                               public IncrementOperators<_Type, ReferenceActiveType<_Type>> {
    public:

      /// See ReferenceActiveType
      using Type = CODI_DECLARE_DEFAULT(_Type, CODI_TEMPLATE(LhsExpressionInterface<double, double, CODI_ANY, CODI_ANY>));
      using Tape = typename Type::Tape; ///< See LhsExpressionInterface.

      using Real = typename Tape::Real;              ///< See LhsExpressionInterface
      using PassiveReal = RealTraits::PassiveReal<Real>;     ///< Basic computation type
      using Identifier = typename Tape::Identifier;  ///< See LhsExpressionInterface
      using Gradient = typename Tape::Gradient;      ///< See LhsExpressionInterface

  private:

      Type& reference;

  public:

      /// Used by Jacobian tapes to optimize for reoccurring arguments.
      mutable Real jacobian;

      /// Constructor
      CODI_INLINE ReferenceActiveType(Type & v) : reference(v), jacobian() {}

      /// See LhsExpressionInterface::operator =(ExpressionInterface const&)
      CODI_INLINE ReferenceActiveType<Tape>& operator=(ReferenceActiveType<Tape > const& v) {
        static_cast<LhsExpressionInterface<Real, Gradient, Tape, ReferenceActiveType>&>(*this) = v;
        return *this;
      }
      using LhsExpressionInterface<Real, Gradient, Tape, ReferenceActiveType>::operator=;

      /*******************************************************************************/
      /// @name Implementation of LhsExpressionInterface
      /// @{

      using StoreAs = ReferenceActiveType const&; ///< \copydoc codi::ExpressionInterface::StoreAs

      /// \copydoc codi::LhsExpressionInterface::getIdentifier()
      CODI_INLINE Identifier& getIdentifier() {
        return reference.getIdentifier();
      }

      /// \copydoc codi::LhsExpressionInterface::getIdentifier() const
      CODI_INLINE Identifier const& getIdentifier() const {
        return reference.getIdentifier();
      }

      /// \copydoc codi::LhsExpressionInterface::value()
      CODI_INLINE Real& value() {
        return reference.value();
      }

      /// \copydoc codi::LhsExpressionInterface::value() const
      CODI_INLINE Real const& value() const {
        return reference.value();
      }

      /// \copydoc codi::LhsExpressionInterface::getGlobalTape()
      static CODI_INLINE Tape& getGlobalTape() {
        return Type::getGlobalTape();
      }
  };
}
