// -----------------------------------------------------------------------------
//
// Copyright (C) The BioDynaMo Project.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
//
// See the LICENSE file distributed with this work for details.
// See the NOTICE file distributed with this work for additional information
// regarding copyright ownership.
//
// -----------------------------------------------------------------------------

#ifndef CORE_AGENT_AGENT_POINTER_H_
#define CORE_AGENT_AGENT_POINTER_H_

#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>
#include <mutex>
#include "core/agent/agent_uid.h"
#include "core/execution_context/in_place_exec_ctxt.h"
#include "core/simulation.h"
#include "core/util/root.h"
#include "core/util/spinlock.h"

namespace bdm {

class Agent;

/// Agent pointer. Required to point to an agent with
/// throughout the whole simulation. Raw pointers cannot be used, because
/// an agent might be copied to a different NUMA domain, or if it resides
/// on a different address space in case of a distributed runtime.
/// Benefit compared to AgentHandle is, that the compiler knows
/// the type returned by `Get` and can therefore inline the code from the callee
/// and perform optimizations.
/// @tparam TAgent agent type
template <typename TAgent>
class AgentPointer {
 public:
  explicit AgentPointer(const AgentUid& uid) : uid_(uid) {}

  /// constructs an AgentPointer object representing a nullptr
  AgentPointer() {}

  virtual ~AgentPointer() {}

  AgentPointer(const AgentPointer& other)
    : uid_(other.uid_)
    , cache_agent_(other.cache_agent_)
    , cache_id_(other.cache_id_)
    {}

  uint64_t GetUid() const { return uid_; }

  
  AgentPointer& operator=(const AgentPointer& other) {
    if (this != &other) {
    std::lock_guard<Spinlock> guard(lock_);
      uid_ = other.uid_;
      // reset to invalid cache state to avoid race conditions in op->
      cache_agent_ = nullptr;
      cache_id_ = -1;
    }
    return *this;
  }

  /// Equals operator that enables the following statement `agent_ptr ==
  /// nullptr;`
  bool operator==(std::nullptr_t) const { return uid_ == AgentUid(); }

  /// Not equal operator that enables the following statement `agent_ptr !=
  /// nullptr;`
  bool operator!=(std::nullptr_t) const { return !this->operator==(nullptr); }

  bool operator==(const AgentPointer& other) const {
    return uid_ == other.uid_;
  }

  bool operator!=(const AgentPointer& other) const {
    return uid_ != other.uid_;
  }

  template <typename TOtherAgent>
  bool operator==(const TOtherAgent* other) const {
    return uid_ == other->GetUid();
  }

  template <typename TOtherAgent>
  bool operator!=(const TOtherAgent* other) const {
    return !this->operator==(other);
  }

  /// Assignment operator that changes the internal representation to nullptr.
  /// Makes the following statement possible `agent_ptr = nullptr;`
  AgentPointer& operator=(std::nullptr_t) {
    std::lock_guard<Spinlock> guard(lock_);
    uid_ = AgentUid();
    cache_agent_ = nullptr;
    cache_id_ = -1;
    return *this;
  }

  TAgent* operator->() {
    assert(*this != nullptr);
    auto* local_cache_agent = cache_agent_; 
    auto local_cache_id = cache_id_; 
    auto* ctxt = Simulation::GetActive()->GetExecutionContext();
    if (local_cache_agent && ctxt->IsCacheValid(local_cache_id)) {
      return local_cache_agent;
    }
    std::lock_guard<Spinlock> guard(lock_);
    auto* agent =  Cast<Agent, TAgent>(ctxt->GetAgent(uid_, &cache_id_));
    cache_agent_ = agent;
    return agent;
  }

  const TAgent* operator->() const {
    assert(*this != nullptr);
    auto* local_cache_agent = cache_agent_; 
    auto local_cache_id = cache_id_; 
    auto* ctxt = Simulation::GetActive()->GetExecutionContext();
    if (local_cache_agent && ctxt->IsCacheValid(local_cache_id)) {
      return const_cast<const TAgent*>(local_cache_agent);
    }
    std::lock_guard<Spinlock> guard(lock_);
    auto* agent =  Cast<const Agent, const TAgent>(ctxt->GetConstAgent(uid_, &cache_id_));
    cache_agent_ = const_cast<TAgent*>(agent); 
    return agent;
  }

  friend std::ostream& operator<<(std::ostream& str,
                                  const AgentPointer& agent_ptr) {
    str << "{ uid: " << agent_ptr.uid_ << "}";
    return str;
  }

  TAgent& operator*() { return *(this->operator->()); }

  const TAgent& operator*() const { return *(this->operator->()); }

  operator bool() const { return *this != nullptr; }  // NOLINT

  TAgent* Get() { return this->operator->(); }

  const TAgent* Get() const { return this->operator->(); }

 private:
  AgentUid uid_;
  mutable TAgent* cache_agent_ = nullptr;
  mutable int64_t cache_id_ = -1;
  mutable Spinlock lock_;

  template <typename TFrom, typename TTo>
  typename std::enable_if<std::is_base_of<TFrom, TTo>::value, TTo*>::type Cast(
      TFrom* agent) const {
    return static_cast<TTo*>(agent);
  }

  template <typename TFrom, typename TTo>
  typename std::enable_if<!std::is_base_of<TFrom, TTo>::value, TTo*>::type Cast(
      TFrom* agent) const {
    return dynamic_cast<TTo*>(agent);
  }

  BDM_CLASS_DEF(AgentPointer, 2);
};

template <typename T>
struct is_agent_ptr {                   // NOLINT
  static constexpr bool value = false;  // NOLINT
};

template <typename T>
struct is_agent_ptr<AgentPointer<T>> {  // NOLINT
  static constexpr bool value = true;   // NOLINT
};

namespace detail {

struct ExtractUid {
  template <typename T>
  static typename std::enable_if<is_agent_ptr<T>::value, uint64_t>::type GetUid(
      const T& t) {
    return t.GetUid();
  }

  template <typename T>
  static typename std::enable_if<!is_agent_ptr<T>::value, uint64_t>::type
  GetUid(const T& t) {
    return 0;  // std::numeric_limits<uint64_t>::max();
  }
};

}  // namespace detail

}  // namespace bdm

#endif  // CORE_AGENT_AGENT_POINTER_H_
