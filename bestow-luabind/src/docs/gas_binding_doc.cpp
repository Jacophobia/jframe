// bestow-luabind/src/docs/gas_binding_doc.cpp
// API documentation for bestow.gas

module bestow.luabind;

import std;

namespace bestow {

void registerGASDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "gas";
    sys.qualifiedName = "bestow.gas";
    sys.description = "Gameplay Ability System (GAS). Manages abilities, attributes, and gameplay effects for entities. This system is not yet implemented -- bindings are coming soon.";

    registry.addSystem(std::move(sys));
}

} // namespace bestow
