#pragma once

#include <vector>

#include "../../aux/macros.hpp"
#include "../../config.h"
#include "../aux/tapeValues.hpp"
#include "chunk.hpp"
#include "position.hpp"

/** \copydoc codi::Namespace */
namespace codi {

  /**
   * @brief Data stream interface for tape data. Encapsulates data that is written e.g. for each statement or argument.
   *
   * This interface defines the basic abstraction mechanism of how data is stored in an AD tape. During the recording of
   * an AD tape, different types of data need to be stored and will be written with a different interval. There can be
   * e.g data for each statement and data for each argument. Each DataInterface covers the data that is written with one
   * specific interval. The management of multiple data streams can become quite cumbersome, therefore the
   * DataInterface is designed in a recursive fashion. Each data stream can be nested with an other data stream, such
   * that they can exchange position information and synchronize themselves.
   *
   * A data item on the data stream can consist of multiple entries. That is, a data entry can be an int or it can be an
   * int and a double. The underlying implementation defines how this data is stored. E.g. as an array of objects or as
   * an object of arrays. For counting the number of items each call to pushData() counts as one item regardless of how
   * many entries each item has.
   *
   * The getPosition() function will produce a position for this DataInterface and all nested DataInterfaces. All
   * methods that have the Position type as an argument or will modify the position of the DataInterface will work
   * recursively on all nested DataInterfaces.
   *
   * See \ref Developer_Simple_Tape for an usage of this interface.
   *
   * Example usage:
   *  \code{.cpp}
   *    ArgDataInterface argVector =...;
   *    StmtDataInterface<ArgDataInterface> stmtVector =...;  // argVector is nested into the stmtVector
   *
   *    stmtVector.setNested(&argVector);  // Set the pointer to the nested vector
   *
   *    // Record some data
   *    // 1. Request space, from child to parent
   *    argVector.reserveItems(2);
   *    stmtVector.reserveItems(1);
   *
   *    // 2. Add the data, any order
   *    argVector.pushData(1.0);
   *    stmtVector.pushData(100);
   *    argVector.pushData(2.0);
   *
   *    // Perform some operations on the data
   *    // How data is provided to func depends on the interface implementations, lets assume start,end and a data
   *    // pointer
   *    auto func = (startStmt, endStmt, stmt, startArg, endArg, arg) {
   *        for (int i = startStmt; i < endStmt; i += 1) { std::cout << st,t[i] << ", ";}
   *        for (int i = startArg; i < endArg; i += 1) { std::cout << arg[i] << ", ";}
   *    };
   *    stmtVector.evaluateForward(stmtVector.getZeroPosition(), stmtVector.getPosition(), func);
   *
   *    // Clear all data vectors
   *    stmtVector.reset();
   *  \endcode
   *
   *
   * The interface defines methods for adding data to the data stream, getting positional information, resetting the
   * data and iterating over the data.
   *
   * Adding data:
   *  - reserveItems(): Needs to be called before a pushData call to ensure that enough space is left.
   *  - pushData():     Add the actual data to the data stream.
   *
   * Positional information:
   *   - getPosition(), getZeroPosition(): Global position of the all nested data interfaces.
   *
   * Reset:
   *   - reset(): Clear data, but do not delete allocated memory.
   *   - resetHard(): Clear data and free allocated memory.
   *   - resetTo(): Clear all data up to the specified position
   *
   * Iterating:
   *   - evaluateForward() / evaluateReverse(): Evaluate from start to end and call the function object for each valid
   *                                          data range in the nested data streams.
   *   - forEachChunk(): Call the function for each data chunk stored by this DataInterface.
   *   - forEachForward() / forEachReverse(): Call a function for each data item in this DataInterface.
   *
   *
   * @tparam _NestedData         Nested data vector needs to implement this DataInterface.
   * @tparam _InternalPosHandle  Position handle of the the implementing class for internal size computations.
   */
  template<typename _NestedData = void, typename _InternalPosHandle = size_t>
  struct DataInterface {
    public:

      using NestedData = CODI_DD(_NestedData, DataInterface);  ///< See DataInterface
      using InternalPosHandle = CODI_DD(_InternalPosHandle, size_t);  ///< See DataInterface

      using Position = EmptyPosition;  ///< Contains position data for this DataInterface and all nested interfaces

      /*******************************************************************************/
      /// @name Adding items

      /**
       * @brief Add data to the allocated data in the implementation. The method can only be called after a call to
       *        reserveItems and only as often as the number of reserved items.
       *
       * pushData() can be called less often than indicated with reserveItems(). The call to reserveItems only
       * represents the maximum number of data items that might be pushed.
       *
       * After a new call to reserveItems() only this many number of data items can be pushed, leftovers will not
       * accumulate.
       *
       * @param[in] data  The number of arguments has to match the number of data stores of the implementation.
       * @tparam Data Types of the pushed data.
       */
      template<typename... Data>
      CODI_INLINE void pushData(Data const&... data);

      /**
       * @brief Reserve this many items on the data stream. See pushData for details.
       *
       * @param[in] items  Number of data items to reserve.
       * @return Can be used in getPushedDataCount(). Only the newest handle is valid.
       */
      CODI_INLINE InternalPosHandle reserveItems(size_t const& items);

      /*******************************************************************************/
      /// @name Size management

      void resize(size_t const& totalSize); /**< Allocated the requested number of data items. */

      void reset(); /**< Reset to the zero position. Data is not deallocated. Also called on nested interfaces. */
      void resetHard(); /**< Reset to the zero position. Data is deallocated and the default size is allocated again.
                             Also called on nested interfaces. */
      void resetTo(Position const& pos); /**< Reset to the given position. Data is not deallocated. Also called on the
                                              nested interfaces. */

      /*******************************************************************************/
      /// @name Position functions

      CODI_INLINE size_t getDataSize() const; /**< @return Total number of data items stored. */
      CODI_INLINE Position getPosition() const; /**< @return The current global position of this DataInterface and all
                                                             nested interfaces */
      CODI_INLINE size_t getPushedDataCount(InternalPosHandle const& startPos); /**< Compute the number of data items
                                                                                     stored after a call to
                                                                                     #reserveItems. */
      CODI_INLINE Position getZeroPosition() const; /**< @return The start position of the DataInterface and all nested
                                                                 interfaces */

      /*******************************************************************************/
      /// @name Misc functions

      /**
       * @brief Add amount of stored data to the TapeValues object. Not called on the nested vector.
       * @param[inout] values  Will only create new data entries and no new section.
       */
      void addToTapeValues(TapeValues& values) const;

      /**
       * @brief Extract the position of a nested DataInterface from the global position object provide by this
       * interface.
       *
       * @param[in] pos  Position of the DataInterface.
       * @tparam TargetPosition  Position definition of a nested DataInterface.
       */
      template<typename TargetPosition>
      CODI_INLINE TargetPosition extractPosition(Position const& pos) const;

      void setNested(NestedData* v); /**< Set the pointer to the nested vector. Needs to be done before any other action
                                          and only once. */
      void swap(DataInterface& other); /**< Swap with other DataInterface of the same type. */

      /*******************************************************************************/
      /// @name Iterator functions

      /**
       * @brief Evaluates the function object with segments of continuous and valid data for all nested DataInterfaces.
       *
       * func is called for each region of continuous data that is valid for all nested DataInterfaces. func is then
       * called as
       * \code{.cpp}
       *   function(args...,
       *            start, end, dataEntry1*, dataEntry2*,
       *            startNested, endNested, dataEntry1Nested*, dataEntry2Nested*,
       *            startNestedNested, endNestedNested, dataEntry1NestedNested*,
       *           ...);
       * \endcode
       *
       * What kind of data is appended by each DataInterface is implementation dependent. The default is the set
       * (start, end, dataEntry1, dataEntry2, etc.) which is appended.
       *
       * It has to hold start <= end.
       *
       * Positions count up in this call.
       *
       * @param[in]    start     Starting position.
       * @param[in]    end       Ending position.
       * @param[in]    function  Function object called.
       * @param[inout] args      Additional arguments for the function object
       *
       * @tparam FunctionObject  Function object which is called.
       * @tparam Args            Arguments for the function object.
       */
      template<typename FunctionObject, typename... Args>
      CODI_INLINE void evaluateForward(Position const& start, Position const& end, FunctionObject function,
                                       Args&&... args);

      /**
       * @brief \copybrief evaluateForward
       *
       * Same as #evaluateForward. It has to hold start >= end. Positions count down in this call.
       *
       * @param[in]    start     Starting position.
       * @param[in]    end       Ending position.
       * @param[in]    function  Function object called.
       * @param[inout] args      Additional arguments for the function object
       *
       * @tparam FunctionObject  Function object which is called.
       * @tparam Args            Arguments for the function object.
       */
      template<typename FunctionObject, typename... Args>
      CODI_INLINE void evaluateReverse(Position const& start, Position const& end, FunctionObject function,
                                       Args&&... args);

      /**
       * @brief Calls the function object for each continuous segment of data.
       *
       * The call is
       * \code
       *  function(chunk, args...);
       * \endcode
       *
       * Chunk is of the type ChunkBase.
       *
       * @param[in]    function   Function object called.
       * @param[in]    recursive  True if same call is done for all nested DataInterfaces.
       * @param[inout] args       Additional arguments for the function object.
       *
       * @tparam FunctionObject  Function object which is called.
       * @tparam Args            Arguments for the function object.
       */
      template<typename FunctionObject, typename... Args>
      CODI_INLINE void forEachChunk(FunctionObject& function, bool recursive, Args&&... args);

      /**
       * @brief Calls the function object for each item in the data stream. This call is not recursive.
       *
       * The call to function is
       * \code{.cpp}
       *   function(args..., entry1*, entry2*,...);
       * \endcode
       *
       * It has to hold start <= end.
       *
       * @param[in]    start      Starting position.
       * @param[in]    end        Ending position
       * @param[in]    function   Function object called.
       * @param[inout] args       Additional arguments for the function object.
       *
       * @tparam FunctionObject  Function object which is called.
       * @tparam Args            Arguments for the function object.
       */
      template<typename FunctionObject, typename... Args>
      CODI_INLINE void forEachForward(Position const& start, Position const& end, FunctionObject function,
                                      Args&&... args);

      /**
       * \copybrief forEachForward
       *
       * See #forEachForward.
       *
       * It has to hold start >= end.
       *
       * @param[in]    start      Starting position.
       * @param[in]    end        Ending position
       * @param[in]    function   Function object called.
       * @param[inout] args       Additional arguments for the function object.
       *
       * @tparam FunctionObject  Function object which is called.
       * @tparam Args            Arguments for the function object.
       */
      template<typename FunctionObject, typename... Args>
      CODI_INLINE void forEachReverse(Position const& start, Position const& end, FunctionObject function,
                                      Args&&... args);
  };
}
