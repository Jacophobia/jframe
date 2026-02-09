// bestow-luabind/src/docs/DocRegistration.cpp
// Master registration — calls all per-system doc registration functions

module bestow.luabind;

import std;

namespace bestow {

DocRegistry createFullDocRegistry() {
    DocRegistry registry;

    registerTypesDoc(registry);
    registerCoreDoc(registry);
    registerInputDoc(registry);
    registerActionDoc(registry);
    registerAudioDoc(registry);
    registerPhysicsDoc(registry);
    registerPhysics3DDoc(registry);
    registerGraphics3DDoc(registry);
    registerAnimationDoc(registry);
    registerCharacterDoc(registry);
    registerEntityDoc(registry);
    registerConfigDoc(registry);
    registerAssetsDoc(registry);
    registerGASDoc(registry);
    registerSceneDoc(registry);
    registerBlueprintsDoc(registry);
    registerGamestateDoc(registry);
    registerStateDoc(registry);
    registerEventsDoc(registry);
    registerUIDoc(registry);
    registerMetricsDoc(registry);
    registerTimerDoc(registry);

    return registry;
}

} // namespace bestow
