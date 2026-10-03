// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/component.h>
#include <generic/dag_initOnDemand.h>
#include <util/dag_string.h>

#include <ska_hash_map/flat_hash_map2.hpp>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class DataBlock;

namespace ecs
{

    typedef std::vector<std::pair<std::string, ecs::ChildComponent>> ComponentsList;
    typedef std::vector<std::string> SceneScriptsList;

    class SceneManager;
    class Scene
    {
    public:
        struct EntityRecord
        {
            ComponentsList clist;
            uint32_t order : 31;
            uint32_t toBeSaved : 1;
            std::string templateName;
            EntityRecord(ComponentsList&& cl, uint32_t o, const char* tn, bool tbs) :
                clist(std::move(cl)),
                order(o),
                toBeSaved(tbs ? 1u : 0u),
                templateName(tn)
            {
            }
        };
        typedef ska::flat_hash_map<EntityId, EntityRecord, EidHash> EMap;

        struct ImportRecord
        {
            static constexpr uint32_t TOP_IMPORT_ORDER = 0xFFFFFFFFUL;
            std::string importScenePath;
            uint32_t order;
        };
        typedef std::vector<ImportRecord> ImportScenesList;

        typename EMap::const_iterator begin() const
        {
            return entities.begin();
        }
        typename EMap::const_iterator end() const
        {
            return entities.end();
        }
        int entitiesCount()
        {
            return entities.size();
        }
        const EntityRecord* findEntityRecord(ecs::EntityId eid) const;
        const ecs::ComponentsList* findComponentsList(ecs::EntityId eid) const;
        EntityRecord* findEntityRecordForModify(ecs::EntityId eid);
        void eraseEntityRecord(ecs::EntityId eid);
        void insertEmptyEntityRecord(ecs::EntityId eid, const char* tname);
        void insertEntityRecord(ecs::EntityId eid, const char* tname, ComponentsList const& comps);
        void cloneEntityRecord(ecs::EntityId source_eid, ecs::EntityId dest_eid, const char* template_name);

        const ImportScenesList& getImportsRecordList() const
        {
            return imports;
        }

        bool hasUnsavedChanges() const
        {
            return unsavedChanges;
        }
        void setNewChangesApplied()
        {
            unsavedChanges = true;
        }
        void setAllChangesWereSaved()
        {
            unsavedChanges = false;
        }

        /// <summary>
        /// Unloads all scripts that wal loaded during SceneManager::StartScene
        /// </summary>
        void ClearAllScripts();

    private:
        void clear()
        {
            entities.clear();
            imports.clear();
            unsavedChanges = false;
            orderSequence = 0;
        }

        EMap entities;
        std::vector<ecs::EntityId> tempEntities;

        uint32_t orderSequence = 0;

        std::string scenePath;

        ImportScenesList imports;
        SceneScriptsList initialSceneScripts;
        SceneScriptsList runtimeSceneScripts;

        bool unsavedChanges = false;

        friend class SceneManager;
    };

class SceneManager
{
public:

  /// <summary>
  /// Determines if we should run scene scripts on scene loading
  /// </summary>
  static bool AutoRunScriptsOnLevelLoad;  // TODO: We need a better way to differentiate editor mode and game mode


  Scene &getActiveScene() { return scene; }

  /// <summary>
  /// Loads scene from blk file
  /// </summary>
  /// <param name="path">Path to blk file</param>
  /// <returns></returns>
  bool loadScene(const char *path);

  /// <summary>
  /// Loads scene from blk
  /// </summary>
  /// <param name="sceneBlk">blk with scene data</param>
  /// <param name="path">Scene::scenePath would be setted from this, if it's not empty</param>
  /// <returns></returns>
  bool loadScene(DataBlock& sceneBlk, const char* path);
  
  /// <summary>
  /// Reloads current scene using it's scene path
  /// </summary>
  void ReloadScene();

  /// <summary>
  /// Unloads current scene
  /// </summary>
  void UnloadScene();

  /// <summary>
  /// Reloads scene depending on EditorSwitchSceneOnInterModeExit parameter in settings:
  /// true  -> sceneload::switch_scene would be called
  /// false -> ReloadSceneFromBlk would be called
  /// </summary>
  /// <param name="fileName">Path to scene blk file</param>
  void ReloadSceneFromFile(const char* fileName = nullptr);
  
  /// <summary>
  /// Remove dynamic entities and spawned entities
  /// Remove all scene scripts with their systems during clearScene
  /// And then calls loadScene
  /// </summary>
  /// <param name="data">blk with scene data</param>
  /// <param name="fileName">path to blk, could be nullptr or empty</param>
  void ReloadSceneFromBlk(DataBlock& data, const char* fileName);

  /// <summary>
  /// Run scripts that belongs to the scene
  /// </summary>
  void StartScene();

  /// <summary>
  /// Clears scripts and scene
  /// </summary>
  void clearScene();

  /// <summary>
  /// Saves current scene to blk then saves it to file 
  /// </summary>
  /// <param name="fpath"></param>
  /// <returns></returns>
  int   SaveCurrentSceneToFile(const char *fpath);

  /// <summary>
  /// Current scene would be saved to provided DataBlock
  /// </summary>
  /// <param name="inOutblk"> DataBlock where scene would be saved </param>
  /// <returns> count of saved objects</returns>
  int SaveCurrentSceneToBlk(DataBlock& inOutblk);

  /// <summary>
  /// Returns list of scripts in [scene].blk file
  /// </summary>
  /// <returns></returns>
  const SceneScriptsList& GetSceneInitScripts() const;

  /// <summary>
  /// Better do that after StartScene was called
  /// </summary>
  /// <returns></returns>
  const SceneScriptsList& GetSceneRuntimeScripts() const;

  /// <summary>
  /// Called on the end of level loading
  /// </summary>
  void OnLevelLoaded();

  /// <summary>
  /// Add initial script to scene
  /// </summary>
  /// <returns></returns>
  void AddSceneInitScript(const std::string& script);

  /// <summary>
  /// Remembers entities created before this function was called
  /// Better do that before loadScene was called
  /// </summary>
  /// <returns></returns>
  void CacheInitialEntities();


protected:

  /// <summary>
  /// Saves entity to scene blk
  /// </summary>
  /// <param name="blk">blk with scene data</param>
  /// <param name="erec">entity record from scene</param>
  void SaveEntityRecord(DataBlock &blk, const ecs::Scene::EntityRecord &erec);


private:
  Scene scene;
  std::unordered_set<ecs::EntityId, ecs::EidHash> initialEntities;
  //ecs::EntityManager* sceneEntMgr;
  friend class Scene;
};
extern InitOnDemand<SceneManager> g_scenes;

inline Scene::EntityRecord *Scene::findEntityRecordForModify(ecs::EntityId eid)
{
  auto it = entities.find(eid);
  return (it != entities.end()) ? &it->second : nullptr;
}

inline const Scene::EntityRecord *Scene::findEntityRecord(ecs::EntityId eid) const
{
  return const_cast<Scene *>(this)->findEntityRecordForModify(eid);
}

inline const ecs::ComponentsList *Scene::findComponentsList(ecs::EntityId eid) const
{
  auto erec = findEntityRecord(eid);
  return erec ? &erec->clist : nullptr;
}

inline void Scene::eraseEntityRecord(ecs::EntityId eid)
{
  const auto *erec = findEntityRecord(eid);
  const bool toBeSaved = erec != nullptr && erec->toBeSaved != 0;
  if (entities.erase(eid))
    if (toBeSaved)
      setNewChangesApplied();
}

inline void Scene::insertEmptyEntityRecord(ecs::EntityId eid, const char *tname)
{
  entities.emplace(eid, Scene::EntityRecord{ComponentsList{}, orderSequence++, tname, /*toBeSaved*/ true});
  setNewChangesApplied();
}

inline void Scene::insertEntityRecord(ecs::EntityId eid, const char *tname, ComponentsList const &comps)
{
  entities.emplace(eid, Scene::EntityRecord{ComponentsList(comps), orderSequence++, tname, /*toBeSaved*/ true});
  setNewChangesApplied();
}

inline void Scene::cloneEntityRecord(ecs::EntityId source_eid, ecs::EntityId dest_eid, const char *template_name)
{
  auto it = entities.find(source_eid);
  if (it != entities.end())
  {
    EntityRecord rec{ComponentsList(it->second.clist), orderSequence++,
      template_name ? template_name : it->second.templateName.c_str(),
      /*toBeSaved*/ true};
    entities.insert_or_assign(dest_eid, std::move(rec));
    setNewChangesApplied();
  }
}

} // namespace ecs
