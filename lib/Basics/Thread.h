////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Achim Brandt
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_BASICS_THREAD_H
#define ARANGODB_BASICS_THREAD_H 1

#include "Basics/Common.h"

#include "Basics/WorkDescription.h"
#include "Basics/threads.h"

namespace arangodb {
namespace velocypack {
class Builder;
}
}

namespace arangodb {
namespace basics {
class ConditionVariable;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief thread
///
/// Each subclass must implement a method run. A thread can be started by
/// start and is stopped either when the method run ends or when stop is
/// called.
////////////////////////////////////////////////////////////////////////////////

class Thread {
  Thread(Thread const&) = delete;
  Thread& operator=(Thread const&) = delete;

 public:
#if defined(TRI_HAVE_POSIX_THREADS)
  typedef pthread_t thread_t;
#elif defined(TRI_HAVE_WIN32_THREADS)
  typedef HANDLE thread_t;
#else
#error OS not supported
#endif

  enum class ThreadState { CREATED, STARTED, STOPPING, STOPPED, DETACHED };

  static std::string stringify(ThreadState);

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief current work description as thread local variable
  //////////////////////////////////////////////////////////////////////////////

  static thread_local Thread* CURRENT_THREAD;

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief allows starting threads
  //////////////////////////////////////////////////////////////////////////////

  static void allowThreadCreation();
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief disallows starting threads
  //////////////////////////////////////////////////////////////////////////////

  static void disallowThreadCreation();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the process id
  //////////////////////////////////////////////////////////////////////////////

  static TRI_pid_t currentProcessId();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the thread number
  ///
  /// Returns a number that uniquely identifies the current thread. If threads
  /// are implemented using processes, this will return a process identifier.
  /// Otherwise it might just return a unique number without any additional
  /// meaning.
  ///
  /// Note that there is a companion function "threadNumber", which returns
  /// the thread number of a running thread.
  //////////////////////////////////////////////////////////////////////////////

  static uint64_t currentThreadNumber();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the thread id
  //////////////////////////////////////////////////////////////////////////////

  static TRI_tid_t currentThreadId();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the current thread
  //////////////////////////////////////////////////////////////////////////////

  static Thread* current() { return CURRENT_THREAD; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief check if work has been canceled
  //////////////////////////////////////////////////////////////////////////////

  static bool isCanceled() {
    Thread* thread = CURRENT_THREAD;

    if (thread == nullptr) {
      return false;
    }

    WorkDescription* desc = thread->workDescription();

    return desc == nullptr ? false : desc->_canceled.load();
  }

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief constructs a thread
  //////////////////////////////////////////////////////////////////////////////

  explicit Thread(std::string const& name, bool allowCreationDuringPrepare = false);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief deletes the thread
  //////////////////////////////////////////////////////////////////////////////

  virtual ~Thread();

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the thread is chatty on shutdown
  //////////////////////////////////////////////////////////////////////////////

  virtual bool isSilent() { return false; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief flags the thread as stopping
  //////////////////////////////////////////////////////////////////////////////

  virtual void beginShutdown();

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief called from the destructor
  //////////////////////////////////////////////////////////////////////////////

  void shutdown();

 public:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief name of a thread
  //////////////////////////////////////////////////////////////////////////////

  std::string const& name() const { return _name; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the thread number
  ///
  /// See currentThreadNumber().
  //////////////////////////////////////////////////////////////////////////////

  uint64_t threadNumber() const { return _threadNumber; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the system thread identifier
  //////////////////////////////////////////////////////////////////////////////

  TRI_tid_t threadId() const { return _threadId; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief true, if the thread is still running
  //////////////////////////////////////////////////////////////////////////////

  bool isRunning() const {
    return _state.load(std::memory_order_relaxed) != ThreadState::STOPPED;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief checks if the current thread was asked to stop
  //////////////////////////////////////////////////////////////////////////////

  bool isStopping() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief starts the thread
  //////////////////////////////////////////////////////////////////////////////

  bool start(basics::ConditionVariable* _finishedCondition = nullptr);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief sets the process affinity
  //////////////////////////////////////////////////////////////////////////////

  void setProcessorAffinity(size_t c);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the current work description
  //////////////////////////////////////////////////////////////////////////////

  WorkDescription* workDescription() const { return _workDescription.load(); }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief sets the current work description
  //////////////////////////////////////////////////////////////////////////////

  void setWorkDescription(WorkDescription*);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief sets the previous work description
  //////////////////////////////////////////////////////////////////////////////

  WorkDescription* setPrevWorkDescription();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief generates a description of the thread
  //////////////////////////////////////////////////////////////////////////////

  virtual void addStatus(arangodb::velocypack::Builder* b);

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief the thread program
  //////////////////////////////////////////////////////////////////////////////

  virtual void run() = 0;

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief static started with access to the private variables
  //////////////////////////////////////////////////////////////////////////////

  static void startThread(void* arg);

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief runner
  //////////////////////////////////////////////////////////////////////////////

  void runMe();

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief name of the thread
  //////////////////////////////////////////////////////////////////////////////

  std::string const _name;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief thread pointer
  //////////////////////////////////////////////////////////////////////////////

  thread_t _thread;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief the thread number
  //////////////////////////////////////////////////////////////////////////////

  uint64_t _threadNumber;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief thread identifier
  //////////////////////////////////////////////////////////////////////////////

  TRI_tid_t _threadId;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief condition variable for done
  //////////////////////////////////////////////////////////////////////////////

  basics::ConditionVariable* _finishedCondition;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief thread has started
  //////////////////////////////////////////////////////////////////////////////

  std::atomic<ThreadState> _state;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief processor affinity
  //////////////////////////////////////////////////////////////////////////////

  int _affinity;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief current work description
  //////////////////////////////////////////////////////////////////////////////

  std::atomic<WorkDescription*> _workDescription;
};
}

#endif
