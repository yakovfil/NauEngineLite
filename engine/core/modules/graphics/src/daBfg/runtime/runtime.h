// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "dabfg/backend/intermediateRepresentation.h"
#include "dabfg/backend/nodeScheduler.h"
#include "dabfg/backend/resourceScheduling/resourceScheduler.h"
#include "dabfg/frontend/dependencyDataCalculator.h"
#include "dabfg/frontend/internalRegistry.h"
#include "dabfg/frontend/irGraphBuilder.h"
#include "dabfg/frontend/nameResolver.h"
#include "dabfg/frontend/nodeTracker.h"
#include "dabfg/runtime/compilationStage.h"
#include "dabfg/runtime/nodeExecutor.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/utils/init_on_demand.h"

template <typename, bool>
struct InitOnDemand;

namespace dabfg
{

class Runtime
{
  friend struct InitOnDemand<Runtime, false>;
  static InitOnDemand<Runtime, false> instance;

public:
  // NOTE: it's good to put this here as everything will be inlined,
  // while the address of the instance in static memory will be resolved
  // by the linker and we will have 0 indirections when accessing stuff
  // inside the backend.
  static void startup() { instance.demandInit(); }
  static Runtime &get() { return *instance; }
  static bool isInitialized() { return static_cast<bool>(instance); }
  static void shutdown() { instance.demandDestroy(); }


  // The following functions are called from various APIs to control the library.

  NodeTracker &getNodeTracker() { return nodeTracker; }
  InternalRegistry &getInternalRegistry() { return registry; }
  void updateExternalState(ExternalState state) { nodeExec->externalState = state; }
  void setMultiplexingExtents(multiplexing::Extents extents);
  void runNodes();

  void markStageDirty(CompilationStage stage)
  {
    if (stage < currentStage)
      currentStage = stage;
  }

  void requestCompleteResourceRescheduling();
  void requestCompleteGraphRecompilation();

  // TODO: remove
  void dumpGraph(const std::string& filename) const;

  private:
  CompilationStage currentStage = CompilationStage::UP_TO_DATE;
  multiplexing::Extents currentMultiplexingExtents;

  // === Components of FG backend ===

  // This provider is used by resource handles acquired from
  // resource requests. Should contain all relevant resources when a
  // node gets executed.
  ResourceProvider currentlyProvidedResources;

  // This registry represents the entire user-specified graph with
  // simple encapsulation-less data (at least in theory)
  InternalRegistry registry{currentlyProvidedResources};
  NameResolver nameResolver{registry};


  DependencyDataCalculator dependencyDataCalculator{registry, nameResolver};

  NodeTracker nodeTracker{registry, dependencyDataCalculator.depData};

  IrGraphBuilder irGraphBuilder{registry, dependencyDataCalculator.depData, nameResolver};

  NodeScheduler cullingScheduler{nodeTracker};

  std::unique_ptr<ResourceScheduler> resourceScheduler;

  // ===


  intermediate::Graph intermediateGraph;
  intermediate::Mapping irMapping;
  sd::NodeStateDeltas perNodeStateDeltas;
  ResourceScheduler::EventsCollectionRef allResourceEvents;

  // Deferred init
  std::optional<NodeExecutor> nodeExec;

  uint32_t frameIndex = 0;

private:
  Runtime();
  ~Runtime();

  void updateNodeDeclarations();
  void resolveNames();
  void calculateDependencyData();
  void buildIrGraph();
  void scheduleNodes();
  void recalculateStateDeltas();
  void scheduleResources();
  void initializeHistoryOfNewResources();
};
} // namespace dabfg
