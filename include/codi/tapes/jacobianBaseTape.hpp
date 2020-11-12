#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "../aux/macros.hpp"
#include "../aux/memberStore.hpp"
#include "../config.h"
#include "../expressions/lhsExpressionInterface.hpp"
#include "../expressions/logic/compileTimeTraversalLogic.hpp"
#include "../expressions/logic/traversalLogic.hpp"
#include "../expressions/logic/helpers/forEachTermLogic.hpp"
#include "../expressions/logic/helpers/jacobianComputationLogic.hpp"
#include "../expressions/referenceActiveType.hpp"
#include "../traits/expressionTraits.hpp"
#include "aux/adjointVectorAccess.hpp"
#include "aux/duplicateJacobianRemover.hpp"
#include "data/chunk.hpp"
#include "data/chunkedData.hpp"
#include "indices/indexManagerInterface.hpp"
#include "commonTapeImplementation.hpp"

/** \copydoc codi::Namespace */
namespace codi {


  /**
   * @brief Type definitions for the Jacobian tapes.
   *
   * @tparam _Real          See TapeTypesInterface.
   * @tparam _Gradient      See TapeTypesInterface.
   * @tparam _IndexManager  Index manager for the tape. Needs to implement IndexManagerInterface.
   * @tparam _Data          See TapeTypesInterface.
   */
  template<typename _Real, typename _Gradient, typename _IndexManager, template<typename, typename> class _Data>
  struct JacobianTapeTypes : public TapeTypesInterface {
    public:

      using Real = CODI_DECLARE_DEFAULT(_Real, double); ///< See JacobianTapeTypes.
      using Gradient = CODI_DECLARE_DEFAULT(_Gradient, double); ///< See JacobianTapeTypes.
      using IndexManager = CODI_DECLARE_DEFAULT(_IndexManager, CODI_TEMPLATE(IndexManagerInterface<int>)); ///< See JacobianTapeTypes.
      template<typename Chunk, typename Nested>
      using Data = CODI_DECLARE_DEFAULT(
                      CODI_TEMPLATE(_Data<Chunk, Nested>),
                      CODI_TEMPLATE(DefaultChunkedData<CODI_ANY, Nested>)); ///< See JacobianTapeTypes.

      using Identifier = typename IndexManager::Index; ///< See IndexManagerInterface.

      constexpr static bool IsLinearIndexHandler = IndexManager::IsLinear; ///< True if the index manager is linear.
      constexpr static bool IsStaticIndexHandler = !IsLinearIndexHandler;  ///< For reuse index mangers a static instantiation is used.

      /// Statement chunk is either \<argument size\> (linear management) or \<lhs identifier, argument size\>
      /// (reuse management)
      using StatementChunk = typename std::conditional<
                                 IsLinearIndexHandler,
                                 Chunk1<Config::ArgumentSize>,
                                 Chunk2<Identifier, Config::ArgumentSize>
                               >::type;
      using StatementData = Data<StatementChunk, IndexManager>; ///< Statement data vector.

      using JacobianChunk = Chunk2<Real, Identifier>; ///< Jacobian chunks is \<Jacobian, rhs index\>
      using JacobianData = Data<JacobianChunk, StatementData>;  ///< Jacobian data vector.

      using NestedData = JacobianData; ///< See TapeTypesInterface.
  };

  /**
   * @brief Base for all standard Jacobian tape implementations.
   *
   * This class provides nearly a full implementation of the FullTapeInterface. There are just a few internal methods
   * left which need to be implemented by the final classes. These methods are mainly based on the index management
   * scheme used in the index manager.
   *
   * @tparam _TapeTypes needs to implement JacobianTapeTypes.
   * @tparam _Impl Type of the final implementations
   */
  template<typename _TapeTypes, typename _Impl>
  struct JacobianBaseTape : public CommonTapeImplementation<_TapeTypes, _Impl> {
    public:

      /// See JacobianBaseTape
      using TapeTypes = CODI_DECLARE_DEFAULT(
                          _TapeTypes,
                          CODI_TEMPLATE(JacobianTapeTypes<double, double,
                                                          IndexManagerInterface<int>, DefaultChunkedData>));
      /// See JacobianBaseTape
      using Impl = CODI_DECLARE_DEFAULT(_Impl, CODI_TEMPLATE(FullTapeInterface<double, double, int, EmptyPosition>));

      using Base = CommonTapeImplementation<TapeTypes, Impl>; ///< Base class abbreviation
      friend Base; ///< Allow the base class to call protected and private methods.

      using Real = typename TapeTypes::Real;                  ///< See TapeTypesInterface.
      using Gradient = typename TapeTypes::Gradient;          ///< See TapeTypesInterface.
      using IndexManager = typename TapeTypes::IndexManager;  ///< See JacobianTapeTypes.
      using Identifier = typename TapeTypes::Identifier;      ///< See TapeTypesInterface.

      using StatementData = typename TapeTypes::StatementData;  ///< See JacobianTapeTypes.
      using JacobianData = typename TapeTypes::JacobianData;    ///< See JacobianTapeTypes.

      using PassiveReal = PassiveRealType<Real>;  ///< Basic computation type

      using NestedPosition = typename JacobianData::Position;    ///< See JacobianTapeTypes.
      using Position = typename Base::Position;                  ///< See TapeTypesInterface.

      static bool constexpr AllowJacobianOptimization = true;    ///< See InternalStatementRecordingInterface.
      static bool constexpr HasPrimalValues = false;             ///< See PrimalEvaluationTapeInterface
      static bool constexpr LinearIndexHandling = TapeTypes::IsLinearIndexHandler; ///< See IdentifierInformationTapeInterface
      static bool constexpr RequiresPrimalRestore = false;       ///< See PrimalEvaluationTapeInterface

    protected:

#if CODI_RemoveDuplicateJacobianArguments
      DuplicateJacobianRemover<Real, Identifier> jacobianSorter;  ///< Replacement for jacobianData to remove duplicated Jacobians.
#endif

      MemberStore<IndexManager, Impl, TapeTypes::IsStaticIndexHandler> indexManager; ///< Index manager.
      StatementData statementData; ///< Data stream for statement specific data.
      JacobianData jacobianData;   ///< Data stream for argument specific data.

      std::vector<Gradient> adjoints; ///< Evaluation vector for AD.

    private:

      CODI_INLINE Impl const& cast() const {
        return static_cast<Impl const&>(*this);
      }

      CODI_INLINE Impl& cast() {
        return static_cast<Impl&>(*this);
      }

    protected:

      /*******************************************************************************/
      /// @name Interface definition
      /// @{

      /// Perform a forward evaluation of the tape. Arguments are from the recursive eval methods of the DataInterface.
      template<typename ... Args>
      static void internalEvaluateForward(Args&& ... args);

      /// Perform a reverse evaluation of the tape. Arguments are from the recursive eval methods of the DataInterface.
      template<typename ... Args>
      static void internalEvaluateReverse(Args&& ... args);

      /// Add statement specific data to the data streams.
      void pushStmtData(Identifier const& index, Config::ArgumentSize const& numberOfArguments);

      /// @}

    public:

      /// Constructor
      JacobianBaseTape() :
        Base(),
  #if CODI_RemoveDuplicateJacobianArguments
        jacobianSorter(),
  #endif
        indexManager(0),
        statementData(Config::ChunkSize),
        jacobianData(Config::ChunkSize),
        adjoints(1)
      {
        statementData.setNested(&indexManager.get());
        jacobianData.setNested(&statementData);

        Base::init(&jacobianData);

        Base::options.insert(TapeParameters::AdjointSize);
        Base::options.insert(TapeParameters::JacobianSize);
        Base::options.insert(TapeParameters::LargestIdentifier);
        Base::options.insert(TapeParameters::StatementSize);
      }

      /*******************************************************************************/
      /// @name Functions from GradientAccessTapeInterface
      /// @{

      /// \copydoc codi::GradientAccessTapeInterface::gradient(Identifier const&)
      CODI_INLINE Gradient& gradient(Identifier const& identifier) {
        checkAdjointSize(identifier);

        return adjoints[identifier];
      }

      /// \copydoc codi::GradientAccessTapeInterface::gradient(Identifier const&) const
      CODI_INLINE Gradient const& gradient(Identifier const& identifier) const {
        if(identifier > (Identifier)adjoints.size()) {
          return adjoints[0];
        } else {
          return adjoints[identifier];
        }
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from InternalStatementRecordingInterface
      /// @{

      /// \copydoc codi::InternalStatementRecordingInterface::initIdentifier()
      template<typename Real>
      CODI_INLINE void initIdentifier(Real& value, Identifier& identifier) {
        CODI_UNUSED(value);

        identifier = IndexManager::UnusedIndex;
      }

      /// \copydoc codi::InternalStatementRecordingInterface::destroyIdentifier()
      template<typename Real>
      CODI_INLINE void destroyIdentifier(Real& value, Identifier& identifier) {
        CODI_UNUSED(value);

        indexManager.get().freeIndex(identifier);
      }

      /// @}

    protected:

      /// Pushes Jacobians and indices to the tape
      struct PushJacobianLogic : public JacobianComputationLogic<Real, PushJacobianLogic> {
        public:
          /// General implementation. Checks for invalid and passive values/Jacobians.
          template<typename Node, typename DataVector>
          CODI_INLINE void handleJacobianOnActive(Node const& node, Real jacobian, DataVector& dataVector) {
            CODI_ENABLE_CHECK(Config::CheckZeroIndex, 0 != node.getIdentifier()) {
              CODI_ENABLE_CHECK(Config::IgnoreInvalidJacobies, isTotalFinite(jacobian)) {
                CODI_ENABLE_CHECK(Config::CheckJacobiIsZero, !isTotalZero(jacobian)) {
                  dataVector.pushData(jacobian, node.getIdentifier());
                }
              }
            }
          }

          /// Specialization for ReferenceActiveType nodes. Delays Jacobian push.
          template<typename Type, typename DataVector>
          CODI_INLINE void handleJacobianOnActive(ReferenceActiveType<Type> const& node, Real jacobian, DataVector& dataVector) {
            CODI_UNUSED(dataVector);

            CODI_ENABLE_CHECK(Config::IgnoreInvalidJacobies, isTotalFinite(jacobian)) {
              // Do a delayed push for these termination nodes, accumulate the jacobian in the local member
              node.jacobian += jacobian;
            }
          }
      };

      /// Pushes all delayed Jacobians.
      struct PushDelayedJacobianLogic : public ForEachTermLogic<PushDelayedJacobianLogic> {
        public:

          /// Specialization for ReferenceActiveType nodes. Pushes the delayed Jacobian.
          template<typename Type, typename DataVector>
          CODI_INLINE void handleActive(ReferenceActiveType<Type> const& node, DataVector& dataVector) {
            CODI_ENABLE_CHECK(Config::CheckZeroIndex, 0 != node.getIdentifier()) {
              CODI_ENABLE_CHECK(Config::CheckJacobiIsZero, !isTotalZero(node.jacobian)) {
                dataVector.pushData(node.jacobian, node.getIdentifier());

                // Reset the jacobian here such that it is not pushed multiple times and ready for the next store
                node.jacobian = Real();
              }
            }
          }

          using ForEachTermLogic<PushDelayedJacobianLogic>::handleActive;
      };

      /// Push Jacobians and delayed Jacobians to the tape.
      template<typename Rhs>
      CODI_INLINE void pushJacobians(ExpressionInterface<Real, Rhs> const& rhs) {
        PushJacobianLogic pushJacobianLogic;
        PushDelayedJacobianLogic pushDelayedJacobianLogic;

#if CODI_RemoveDuplicateJacobianArguments
        auto& insertVector = jacobianSorter;
#else
        auto& insertVector = jacobianData;
#endif

        pushJacobianLogic.eval(rhs.cast(), Real(1.0), insertVector);
        pushDelayedJacobianLogic.eval(rhs.cast(), insertVector);

#if CODI_RemoveDuplicateJacobianArguments
        jacobianSorter.storeData(jacobianData);
#endif
      }

    public:

      /// @{

      /// \copydoc codi::InternalStatementRecordingInterface::store()
      template<typename Lhs, typename Rhs>
      CODI_INLINE void store(LhsExpressionInterface<Real, Gradient, Impl, Lhs>& lhs,
                 ExpressionInterface<Real, Rhs> const& rhs) {

        CODI_ENABLE_CHECK(Config::CheckTapeActivity, cast().isActive()) {
          size_t constexpr MaxArgs = NumberOfActiveTypeArguments<Rhs>::value;

          statementData.reserveItems(1);
          typename JacobianData::InternalPosHandle jacobianStart = jacobianData.reserveItems(MaxArgs);

          pushJacobians(rhs);

          size_t numberOfArguments = jacobianData.getPushedDataCount(jacobianStart);
          if(0 != numberOfArguments) {
            indexManager.get().assignIndex(lhs.cast().getIdentifier());
            cast().pushStmtData(lhs.cast().getIdentifier(), (Config::ArgumentSize)numberOfArguments);
          } else {
            indexManager.get().freeIndex(lhs.cast().getIdentifier());
          }
        } else {
          indexManager.get().freeIndex(lhs.cast().getIdentifier());
        }

        lhs.cast().value() = rhs.cast().getValue();
      }

      /// \copydoc codi::InternalStatementRecordingInterface::store() <br>
      /// Optimization for copy statements.
      template<typename Lhs, typename Rhs>
      CODI_INLINE void store(LhsExpressionInterface<Real, Gradient, Impl, Lhs>& lhs,
                 LhsExpressionInterface<Real, Gradient, Impl, Rhs> const& rhs) {

        CODI_ENABLE_CHECK(Config::CheckTapeActivity, cast().isActive()) {
          if(IndexManager::CopyNeedsStatement || !Config::CopyOptimization) {
            store<Lhs, Rhs>(lhs, static_cast<ExpressionInterface<Real, Rhs> const&>(rhs));
          } else {
            indexManager.get().copyIndex(lhs.cast().getIdentifier(), rhs.cast().getIdentifier());
          }
        } else {
          indexManager.get().freeIndex(lhs.cast().getIdentifier());
        }

        lhs.cast().value() = rhs.cast().getValue();
      }

      /// \copydoc codi::InternalStatementRecordingInterface::store() <br>
      /// Specialization for passive assignments.
      template<typename Lhs>
      CODI_INLINE void store(LhsExpressionInterface<Real, Gradient, Impl, Lhs>& lhs, PassiveReal const& rhs) {
        indexManager.get().freeIndex(lhs.cast().getIdentifier());

        lhs.cast().value() = rhs;
      }


      /// @}
      /*******************************************************************************/
      /// @name Functions from ReverseTapeInterface
      /// @{

      /// \copydoc codi::ReverseTapeInterface::registerInput()
      template<typename Lhs>
      CODI_INLINE void registerInput(LhsExpressionInterface<Real, Gradient, Impl, Lhs>& value) {
        indexManager.get().assignUnusedIndex(value.cast().getIdentifier());

        if(TapeTypes::IsLinearIndexHandler) {
          statementData.reserveItems(1);
          cast().pushStmtData(value.cast().getIdentifier(), Config::StatementInputTag);
        }
      }

      /// \copydoc codi::ReverseTapeInterface::clearAdjoints()
      CODI_INLINE void clearAdjoints() {
        for(Gradient& gradient : adjoints) {
          gradient = Gradient();
        }
      }

      /// @}

    protected:

      /// Adds data from all streams, the size of the adjoint vector and index manager data.
      CODI_INLINE TapeValues internalGetTapeValues() const {
        std::string name;
        if(TapeTypes::IsLinearIndexHandler) {
          name = "CoDi Tape Statistics ( JacobiLinearTape )";
        } else {
          name = "CoDi Tape Statistics ( JacobiReuseTape )";
        }
        TapeValues values = TapeValues(name);

        size_t nAdjoints      = indexManager.get().getLargestAssignedIndex();
        double memoryAdjoints = static_cast<double>(nAdjoints) * static_cast<double>(sizeof(Gradient)) * TapeValues::BYTE_TO_MB;

        values.addSection("Adjoint vector");
        values.addUnsignedLongEntry("Number of adjoints", nAdjoints);
        values.addDoubleEntry("Memory allocated", memoryAdjoints, true, true);

        values.addSection("Index manager");
        indexManager.get().addToTapeValues(values);

        values.addSection("Statement entries");
        statementData.addToTapeValues(values);
        values.addSection("Jacobian entries");
        jacobianData.addToTapeValues(values);

        return values;
      }

      /******************************************************************************
       * Protected helper function for CustomAdjointVectorEvaluationTapeInterface
       */


    protected:

      /// Performs the AD \ref sec_reverseAD "reverse" equation for a statement.
      template<typename Adjoint>
      CODI_INLINE static void incrementAdjoints(
          Adjoint* adjointVector,
          Adjoint const& lhsAdjoint,
          Config::ArgumentSize const& numberOfArguments,
          size_t& curJacobianPos,
          Real const* const rhsJacobians,
          Identifier const* const rhsIdentifiers) {

        size_t endJacobianPos = curJacobianPos - numberOfArguments;

        CODI_ENABLE_CHECK(Config::SkipZeroAdjointEvaluation, !isTotalZero(lhsAdjoint)){
          while(endJacobianPos < curJacobianPos) {
            curJacobianPos -= 1;
            adjointVector[rhsIdentifiers[curJacobianPos]] += rhsJacobians[curJacobianPos] * lhsAdjoint;
          }
        } else {
          curJacobianPos = endJacobianPos;
        }
      }

      /// Wrapper helper for improved compiler optimizations.
      CODI_WRAP_FUNCTION_TEMPLATE(Wrap_internalEvaluateReverse, Impl::template internalEvaluateReverse);

      /// Start for reverse evaluation between external function.
      template<typename Adjoint>
      CODI_NO_INLINE static void internalEvaluateReverseVector(NestedPosition const& start, NestedPosition const& end, Adjoint* data, JacobianData& jacobianData) {
          Wrap_internalEvaluateReverse<Adjoint> evalFunc;
          jacobianData.evaluateReverse(start, end, evalFunc, data);
      }

      /// Performs the AD \ref sec_forwardAD "forward" equation for a statement.
      template<typename Adjoint>
      CODI_INLINE static void incrementTangents (
          Adjoint const* const adjointVector,
          Adjoint& lhsAdjoint,
          Config::ArgumentSize const& numberOfArguments,
          size_t& curJacobianPos,
          Real const* const rhsJacobians,
          Identifier const* const rhsIdentifiers) {

        size_t endJacobianPos = curJacobianPos + numberOfArguments;

        while(curJacobianPos < endJacobianPos) {
          lhsAdjoint += rhsJacobians[curJacobianPos] * adjointVector[rhsIdentifiers[curJacobianPos]];
          curJacobianPos += 1;
        }
      }

      /// Wrapper helper for improved compiler optimizations.
      CODI_WRAP_FUNCTION_TEMPLATE(Wrap_internalEvaluateForward, Impl::template internalEvaluateForward);

      /// Start for forward evaluation between external function.
      template<typename Adjoint>
      CODI_NO_INLINE static void internalEvaluateForwardVector(NestedPosition const& start, NestedPosition const& end, Adjoint* data, JacobianData& jacobianData) {
          Wrap_internalEvaluateForward<Adjoint> evalFunc;
          jacobianData.evaluateForward(start, end, evalFunc, data);
      }

    public:

      /// @name Functions from CustomAdjointVectorEvaluationTapeInterface
      /// @{

      using Base::evaluate;

      /// \copydoc codi::CustomAdjointVectorEvaluationTapeInterface::evaluate()
      template<typename Adjoint>
      CODI_NO_INLINE void evaluate(Position const& start, Position const& end, Adjoint* data) {
        AdjointVectorAccess<Real, Identifier, Adjoint> adjointWrapper(data);

        Base::internalEvaluateExtFunc(start, end, JacobianBaseTape::template internalEvaluateReverseVector<Adjoint>, &adjointWrapper, data, jacobianData);
      }

      /// \copydoc codi::CustomAdjointVectorEvaluationTapeInterface::evaluate()
      template<typename Adjoint>
      CODI_NO_INLINE void evaluateForward(Position const& start, Position const& end, Adjoint* data) {
        AdjointVectorAccess<Real, Identifier, Adjoint> adjointWrapper(data);

        Base::internalEvaluateExtFuncForward(start, end, JacobianBaseTape::template internalEvaluateForwardVector<Adjoint>, &adjointWrapper, data, jacobianData);
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from DataManagementTapeInterface
      /// @{


      /// \copydoc codi::DataManagementTapeInterface::swap()
      CODI_INLINE void swap(Impl& other) {

        // Index manager does not need to be swapped, it is either static or swapped in with the vector data
        // Vectors are swapped recursively in the base class

        std::swap(adjoints, other.adjoints);

        Base::swap(other);
      }

      /// \copydoc codi::DataManagementTapeInterface::deleteAdjointVector()
      void deleteAdjointVector() {
        adjoints.resize(1);
      }

      /// \copydoc codi::DataManagementTapeInterface::getParameter()
      size_t getParameter(TapeParameters parameter) const {
        switch (parameter) {
          case TapeParameters::AdjointSize:
            return adjoints.size();
            break;
          case TapeParameters::JacobianSize:
            return jacobianData.getDataSize();
            break;
          case TapeParameters::LargestIdentifier:
            return indexManager.get().getLargestAssignedIndex();
            break;
          case TapeParameters::StatementSize:
            return statementData.getDataSize();
          default:
            return Base::getParameter(parameter);
            break;
        }
      }

      /// \copydoc codi::DataManagementTapeInterface::setParameter()
      void setParameter(TapeParameters parameter, size_t value) {
        switch (parameter) {
          case TapeParameters::AdjointSize:
            adjoints.resize(value);
            break;
          case TapeParameters::JacobianSize:
            jacobianData.resize(value);
            break;
          case TapeParameters::LargestIdentifier:
            CODI_EXCEPTION("Tried to set a get only parameter.");
            break;
          case TapeParameters::StatementSize:
            statementData.resize(value);
            break;
          default:
            Base::setParameter(parameter, value);
            break;
        }
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from ExternalFunctionTapeInterface
      /// @{

      /// \copydoc codi::ExternalFunctionTapeInterface::registerExternalFunctionOutput()
      template<typename Lhs>
      Real registerExternalFunctionOutput(LhsExpressionInterface<Real, Gradient, Impl, Lhs>& value) {
        cast().registerInput(value);

        return Real();
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from ForwardEvaluationTapeInterface
      /// @{

      using Base::evaluateForward;

      /// \copydoc codi::ForwardEvaluationTapeInterface::evaluateForward()
      void evaluateForward(Position const& start, Position const& end) {

        checkAdjointSize(indexManager.get().getLargestAssignedIndex());

        cast().evaluateForward(start, end, adjoints.data());
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from ManualStatementPushTapeInterface
      /// @{


      /// \copydoc codi::ManualStatementPushTapeInterface::pushJacobiManual()
      void pushJacobiManual(Real const& jacobi, Real const& value, Identifier const& index) {
        CODI_UNUSED(value);

        jacobianData.pushData(jacobi, index);
      }

      /// \copydoc codi::ManualStatementPushTapeInterface::storeManual()
      void storeManual(Real const& lhsValue, Identifier& lhsIndex, Config::ArgumentSize const& size) {
        CODI_UNUSED(lhsValue);

        statementData.reserveItems(1);
        jacobianData.reserveItems(size);

        indexManager.get().assignIndex(lhsIndex);
        cast().pushStmtData(lhsIndex, (Config::ArgumentSize)size);
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from PositionalEvaluationTapeInterface
      /// @{


      /// \copydoc codi::PositionalEvaluationTapeInterface::evaluate()
      CODI_INLINE void evaluate(Position const& start, Position const& end) {
        checkAdjointSize(indexManager.get().getLargestAssignedIndex());

        evaluate(start, end, adjoints.data());
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from PreaccumulationEvaluationTapeInterface
      /// @{

      /// \copydoc codi::PreaccumulationEvaluationTapeInterface::evaluateKeepState()
      void evaluateKeepState(Position const& start, Position const& end) {
        evaluate(start, end);
      }

      /// \copydoc codi::PreaccumulationEvaluationTapeInterface::evaluateForwardKeepState()
      void evaluateForwardKeepState(Position const& start, Position const& end) {
        evaluateForward(start, end);
      }

      /// @}
      /*******************************************************************************/
      /// @name Functions from PrimalEvaluationTapeInterface
      /// @{

      using Base::evaluatePrimal;

      /// Not implemented raises exception.
      void evaluatePrimal(Position const& start, Position const& end) {
        CODI_UNUSED(start, end);

        CODI_EXCEPTION("Accessing primal evaluation of an Jacobian tape.");
      }

      /// Not implemented raises exception.
      Real& primal(Identifier const& identifier) {
        CODI_UNUSED(identifier);

        CODI_EXCEPTION("Accessing primal vector of an Jacobian tape.");

        static Real temp;
        return temp;
      }

      /// Not implemented raises exception.
      Real primal(Identifier const& identifier) const {
        CODI_UNUSED(identifier);

        CODI_EXCEPTION("Accessing primal vector of an Jacobian tape.");

        return Real();
      }

      /// @}

    private:

      CODI_INLINE void checkAdjointSize(Identifier const& identifier) {
        if(identifier >= (Identifier)adjoints.size()) {
          resizeAdjointsVector();
        }
      }

      CODI_NO_INLINE void resizeAdjointsVector() {
        adjoints.resize(indexManager.get().getLargestAssignedIndex() + 1);
      }
  };
}
