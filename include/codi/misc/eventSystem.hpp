/*
 * CoDiPack, a Code Differentiation Package
 *
 * Copyright (C) 2015-2022 Chair for Scientific Computing (SciComp), TU Kaiserslautern
 * Homepage: http://www.scicomp.uni-kl.de
 * Contact:  Prof. Nicolas R. Gauger (codi@scicomp.uni-kl.de)
 *
 * Lead developers: Max Sagebaum, Johannes Blühdorn (SciComp, TU Kaiserslautern)
 *
 * This file is part of CoDiPack (http://www.scicomp.uni-kl.de/software/codi).
 *
 * CoDiPack is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * CoDiPack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU
 * General Public License along with CoDiPack.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * For other licensing options please contact us.
 *
 * Authors:
 *  - SciComp, TU Kaiserslautern:
 *    - Max Sagebaum
 *    - Johannes Blühdorn
 *    - Former members:
 *      - Tim Albring
 */
#pragma once

#include <list>
#include <map>

#include "../tapes/interfaces/fullTapeInterface.hpp"
#include "../config.h"
#include "macros.hpp"

/** \copydoc codi::Namespace */
namespace codi {

  template<typename T_Tape>
  struct EventSystem {
    public:
      using Tape = CODI_DD(T_Tape, CODI_T(FullTapeInterface<double, double, int, EmptyPosition>));
      using Real = typename Tape::Real;
      using Gradient = typename Tape::Gradient;
      using Index = typename Tape::Identifier;
      using Position = typename Tape::Position;

    private:
      enum class Event {
        /* high-level events */
        /* AD workflow */
        TapeStartRecording,
        TapeStopRecording,
        TapeRegisterInput,
        TapeRegisterOutput,
        TapeEvaluate,
        TapeReset,
        /* ... */
        /* low-level events */
        /*... */
        /* index management events */
        IndexAssign
        /* ... */
      };

      using Callback = void*;
      static std::map<Event, std::list<std::pair<Callback, void*>>> listeners;

      template<typename TypedCallback>
      static CODI_INLINE void internalRegisterListener(Event event, TypedCallback callback, void* customData) {
        listeners[event].push_back(std::make_pair((void*)callback, customData));
      }

      template<typename TypedCallback, typename... Args>
      static CODI_INLINE void internalNotifyListeners(bool enabled, Event event, Args&&... args) {
        if (enabled) {
          for (auto const& listener : listeners[event]) {
            ((TypedCallback) listener.first)(std::forward<Args>(args)..., listener.second);
          }
        }
      }

    public:

      /*******************************************************************************/
      /// @name AD workflow
      /// @{

      static CODI_INLINE void registerTapeStartRecordingListener(void (*callback)(Tape&, void*), void* customData = nullptr) {
        internalRegisterListener(Event::TapeStartRecording, callback, customData);
      }

      static CODI_INLINE void notifyTapeStartRecordingListeners(Tape& tape) {
        internalNotifyListeners<void (*)(Tape&, void*)>(Config::HighLevelEvents, Event::TapeStartRecording, tape);
      }

      static CODI_INLINE void registerTapeStopRecordingListener(void (*callback)(Tape&, void*), void* customData = nullptr) {
        internalRegisterListener(Event::TapeStopRecording, callback, customData);
      }

      static CODI_INLINE void notifyTapeStopRecordingListeners(Tape& tape) {
        internalNotifyListeners<void (*)(Tape&, void*)>(Config::HighLevelEvents, Event::TapeStopRecording, tape);
      }

      template<typename Lhs>
      static CODI_INLINE void registerTapeRegisterInputListener(void (*callback)(Tape&, Lhs&, void*), void* customData = nullptr) {
        internalRegisterListener(Event::TapeRegisterInput, callback, customData);
      }

      template<typename Lhs>
      static CODI_INLINE void notifyTapeRegisterInputListeners(Tape& tape, Lhs& value) {
        internalNotifyListeners<void (*)(Tape&, Lhs&, void*)>(Config::HighLevelEvents, Event::TapeRegisterInput, tape, value);
      }

      template<typename Lhs>
      static CODI_INLINE void registerTapeRegisterOutputListener(void (*callback)(Tape&, Lhs&, void*), void* customData = nullptr) {
        internalRegisterListener(Event::TapeRegisterOutput, callback, customData);
      }

      template<typename Lhs>
      static CODI_INLINE void notifyTapeRegisterOutputListeners(Tape& tape, Lhs& value) {
        internalNotifyListeners<void (*)(Tape&, Lhs&, void*)>(Config::HighLevelEvents, Event::TapeRegisterOutput, tape, value);
      }

      template<typename Adjoint>
      static CODI_INLINE void registerTapeEvaluateListener(void (*callback)(Tape&, Position const&, Position const&, Adjoint*, void*), void* customData = nullptr) {
        internalRegisterListener(Event::TapeEvaluate, callback, customData);
      }

      template<typename Adjoint>
      static CODI_INLINE void notifyTapeEvaluateListeners(Tape& tape, Position const& start, Position const& end, Adjoint* adjoint) {
        internalNotifyListeners<void (*)(Tape&, Position const&, Position const&, Adjoint*, void*)>(Config::HighLevelEvents, Event::TapeEvaluate, tape, start, end, adjoint);
      }

      static CODI_INLINE void registerTapeResetListener(void (*callback)(Tape&, Position const&, bool, void*), void* customData = nullptr) {
        internalRegisterListener(Event::TapeReset, callback, customData);
      }

      static CODI_INLINE void notifyTapeResetListeners(Tape& tape, Position const& position, bool clearAdjoints) {
        internalNotifyListeners<void (*)(Tape&, Position const&, bool, void*)>(Config::HighLevelEvents, Event::TapeReset, tape, position, clearAdjoints);
      }

      /// @}

      static CODI_INLINE void registerIndexAssignListener(void (*callback)(Index&, void*), void* customData = nullptr) {
        internalRegisterListener(Event::IndexAssign, callback, customData);
      }

      static CODI_INLINE void notifyIndexAssignListeners(Index& index) {
        internalNotifyListeners<void (*)(Index&, void*)>(Config::LowLevelEvents, Event::IndexAssign, index);
      }
  };

  template<typename Tape>
  std::map<typename EventSystem<Tape>::Event, std::list<std::pair<typename EventSystem<Tape>::Callback, void*>>> EventSystem<Tape>::listeners;
}
