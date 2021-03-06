////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2017 ArangoDB GmbH, Cologne, Germany
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
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_REPLICATION_DATABASE_INITIAL_SYNCER_H
#define ARANGOD_REPLICATION_DATABASE_INITIAL_SYNCER_H 1

#include "Replication/InitialSyncer.h"
#include "Basics/Result.h"
#include "Cluster/ServerState.h"

struct TRI_vocbase_t;

namespace arangodb {

class LogicalCollection;
class DatabaseInitialSyncer;
class ReplicationApplierConfiguration;

class DatabaseInitialSyncer final : public InitialSyncer {
  friend ::arangodb::Result handleSyncKeysMMFiles(DatabaseInitialSyncer& syncer, arangodb::LogicalCollection* col,
                                                              std::string const& keysId);
  friend ::arangodb::Result handleSyncKeysRocksDB(DatabaseInitialSyncer& syncer, arangodb::LogicalCollection* col,
                                                              std::string const& keysId);
  friend ::arangodb::Result syncChunkRocksDB(DatabaseInitialSyncer& syncer, SingleCollectionTransaction* trx,
                                           std::string const& keysId, uint64_t chunkId,
                                           std::string const& lowString,
                                           std::string const& highString,
                                           std::vector<std::pair<std::string, uint64_t>> const& markers);

 private:
  /// @brief apply phases
  typedef enum {
    PHASE_NONE,
    PHASE_INIT,
    PHASE_VALIDATE,
    PHASE_DROP_CREATE,
    PHASE_DUMP
  } sync_phase_e;

 public:
  DatabaseInitialSyncer(
    TRI_vocbase_t& vocbase,
    ReplicationApplierConfiguration const& configuration
  );

  /// @brief run method, performs a full synchronization
  Result run(bool incremental) override {
    return runWithInventory(incremental, velocypack::Slice::noneSlice());
  }

  /// @brief run method, performs a full synchronization with the
  ///        given list of collections.
  Result runWithInventory(bool incremental,
                          velocypack::Slice collections);

  TRI_vocbase_t* resolveVocbase(velocypack::Slice const& slice) override {
    return &_vocbase;
  }

  /// @brief translate a phase to a phase name
  std::string translatePhase(sync_phase_e phase) const {
    switch (phase) {
      case PHASE_INIT:
        return "init";
      case PHASE_VALIDATE:
        return "validate";
      case PHASE_DROP_CREATE:
        return "drop-create";
      case PHASE_DUMP:
        return "dump";
      case PHASE_NONE:
        break;
    }
    return "none";
  }

  TRI_vocbase_t& vocbase() const {
    TRI_ASSERT(vocbases().size() == 1);
    return vocbases().begin()->second.database();
  }

  /// @brief check whether the initial synchronization should be aborted
  bool isAborted() const override;

  /// @brief insert the batch id and barrier ID.
  ///        For use in globalinitalsyncer
  void useAsChildSyncer(Syncer::MasterInfo const& info,
                        uint64_t barrierId, double barrierUpdateTime,
                        uint64_t batchId, double batchUpdateTime) {
    _isChildSyncer = true;
    _masterInfo = info;
    _barrierId = barrierId;
    _barrierUpdateTime = barrierUpdateTime;
    _batchId = batchId;
    _batchUpdateTime = batchUpdateTime;
  }

  /// @brief last time the barrier was extended or created
  /// The barrier prevents the deletion of WAL files for mmfiles
  double barrierUpdateTime() const { return _barrierUpdateTime; }
  
  /// @brief last time the batch was extended or created
  /// The batch prevents compaction in mmfiles and keeps a snapshot
  /// in rocksdb for a constant view of the data
  double batchUpdateTime() const { return _batchUpdateTime; }

 private:

  /// @brief set a progress message
  void setProgress(std::string const& msg) override;

  /// @brief send a WAL flush command
  Result sendFlush();

  /// @brief handle a single dump marker
  Result parseCollectionDumpMarker(transaction::Methods&,
                                   arangodb::LogicalCollection*,
                                   arangodb::velocypack::Slice const&);

  /// @brief apply the data from a collection dump
  Result parseCollectionDump(transaction::Methods&, LogicalCollection* col,
                             httpclient::SimpleHttpResult*, uint64_t&);

  /// @brief determine the number of documents in a collection
  int64_t getSize(arangodb::LogicalCollection*);

  /// @brief incrementally fetch data from a collection
  Result fetchCollectionDump(arangodb::LogicalCollection*,
                             std::string const& leaderColl, TRI_voc_tick_t);

  /// @brief incrementally fetch data from a collection
  Result fetchCollectionSync(arangodb::LogicalCollection*,
                             std::string const& leaderColl, TRI_voc_tick_t);

  /// @brief changes the properties of a collection, based on the VelocyPack
  /// provided
  Result changeCollection(arangodb::LogicalCollection*,
                          arangodb::velocypack::Slice const&);

  /// @brief handle the information about a collection
  Result handleCollection(arangodb::velocypack::Slice const&,
                          arangodb::velocypack::Slice const&, bool incremental,
                          sync_phase_e);

  /// @brief fetch the server's inventory
  Result fetchInventory(arangodb::velocypack::Builder& builder);

  /// @brief handle the inventory response of the master
  Result handleLeaderCollections(arangodb::velocypack::Slice const&, bool);

  /// @brief iterate over all collections from an array and apply an action
  Result iterateCollections(
      std::vector<std::pair<arangodb::velocypack::Slice,
                            arangodb::velocypack::Slice>> const&,
      bool incremental, sync_phase_e);

  std::unordered_map<std::string, std::string> createHeaders() const;

  TRI_vocbase_t& _vocbase;

  /// @brief whether or not the WAL on the remote server has been flushed by us
  bool _hasFlushed;

  /// @brief maximum internal value for chunkSize
  static size_t const MaxChunkSize;
};

} // arangodb

#endif