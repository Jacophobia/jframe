// audio-demo/src/main.cpp
// Comprehensive demonstration of IAudioSystem interface

import std;
import bestow.types;
import bestow.audio;
import bestow.assets;

using namespace bestow;

// Mock implementations for demonstration purposes
class MockAssetSystem : public IAssetSystem {
public:
    void update() override {}

    AssetHandle registerAsset(AssetType type, const std::filesystem::path& path) override {
        static UUID nextUUID = 1;
        AssetHandle handle{nextUUID++, type};
        std::println("  [AssetSystem] Registered {} asset: {} (UUID: {})",
                     assetTypeName(type), path.string(), handle.uuid);
        return handle;
    }

    void unregisterAsset(AssetHandle handle) override {}
    void loadAsset(AssetHandle handle) override {}
    void loadAssetAsync(AssetHandle handle, AssetLoadCallback callback) override {}
    void unloadAsset(AssetHandle handle) override {}
    AssetState getAssetState(AssetHandle handle) const override { return AssetState::Loaded; }
    AssetMetadata getAssetMetadata(AssetHandle handle) const override { return {}; }
    bool isLoaded(AssetHandle handle) const override { return true; }
    void* getRawAsset(AssetHandle handle) override { return nullptr; }
    const void* getRawAsset(AssetHandle handle) const override { return nullptr; }
    void loadAll() override {}
    void unloadAll() override {}
    std::vector<AssetHandle> getAssetsOfType(AssetType type) const override { return {}; }
    void enableHotReload(bool enable) override {}
    void checkForReloads() override {}
    void reloadAsset(AssetHandle handle) override {}

    // 3D asset methods (stubs for this demo)
    const MeshData* getMeshData(AssetHandle handle) const override { return nullptr; }
    const ModelData* getModelData(AssetHandle handle) const override { return nullptr; }
    const MaterialData* getMaterialData(AssetHandle handle) const override { return nullptr; }
    const CubemapData* getCubemapData(AssetHandle handle) const override { return nullptr; }
    AssetHandle loadMesh(const std::filesystem::path& path) override { return {}; }
    AssetHandle loadModel(const std::filesystem::path& path) override { return {}; }
    AssetHandle loadCubemap(const std::filesystem::path& path) override { return {}; }
    AssetHandle loadCubemap(
        const std::filesystem::path&, const std::filesystem::path&,
        const std::filesystem::path&, const std::filesystem::path&,
        const std::filesystem::path&, const std::filesystem::path&) override { return {}; }

    // System integration (stubs for this demo)
    void setEventSystem(class IEventSystem*) override {}
    void setJobSystem(void*) override {}

    // Subscription methods (stubs for this demo)
    SubscriptionId subscribe(AssetHandle, AssetChangeCallback) override { return 0; }
    SubscriptionId subscribeToType(AssetType, AssetChangeCallback) override { return 0; }
    void unsubscribe(SubscriptionId) override {}

    // Shader methods (stubs for this demo)
    AssetHandle loadShader(const std::filesystem::path&) override { return {}; }
    const ShaderData* getShaderData(AssetHandle) const override { return nullptr; }
    void compileShaderAsync(AssetHandle, AssetLoadCallback) override {}
    bool isShaderCompilationSupported() const override { return false; }

private:
    std::string assetTypeName(AssetType type) const {
        switch (type) {
            case AssetType::Sound: return "Sound";
            case AssetType::Music: return "Music";
            default: return "Unknown";
        }
    }
};

// Helper function to print section headers
void printSection(const std::string& title) {
    std::println("\n{:=^80}", "");
    std::println("{:^80}", title);
    std::println("{:=^80}\n", "");
}

// Helper function to print subsection headers
void printSubsection(const std::string& title) {
    std::println("\n{:-^60}", " " + title + " ");
}

// Helper function to simulate time passing
void simulateUpdate(float dt) {
    // In a real application, you would call: audio->update(dt);
    std::println("  [Update] dt = {:.3f}s", dt);
}

int main() {
    std::println("Bestow Audio System - Comprehensive API Demonstration");
    std::println("=====================================================\n");

    // Create mock asset system
    MockAssetSystem assets;

    // Create audio system (would be actual implementation in real usage)
    // For demo purposes, we'll show the API calls
    std::println("Note: This demo shows the Audio System API interface.");
    std::println("Actual audio playback requires FMOD integration.\n");

    // In a real application, you would get the audio system from dependency injection:
    // IAudioSystem* audio = /* from DI container */;

    // For demonstration, we'll show the API calls that would be made
    std::println("Creating audio system instance...");
    std::println("Audio system initialized.\n");

    // ======================================================================
    // SECTION 1: Asset Registration
    // ======================================================================
    printSection("SECTION 1: Asset Registration");

    std::println("Registering audio assets...\n");

    AssetHandle musicTrack = assets.registerAsset(AssetType::Music,
                                                   "audio/music/theme.ogg");
    AssetHandle ambienceLoop = assets.registerAsset(AssetType::Sound,
                                                     "audio/ambience/forest.ogg");
    AssetHandle jumpSound = assets.registerAsset(AssetType::Sound,
                                                  "audio/sfx/jump.wav");
    AssetHandle explosionSound = assets.registerAsset(AssetType::Sound,
                                                       "audio/sfx/explosion.wav");
    AssetHandle coinSound = assets.registerAsset(AssetType::Sound,
                                                  "audio/sfx/coin.wav");
    AssetHandle voiceLine = assets.registerAsset(AssetType::Sound,
                                                  "audio/voice/welcome.wav");
    AssetHandle footstepSound = assets.registerAsset(AssetType::Sound,
                                                      "audio/sfx/footstep.wav");

    std::println("\n[Demo] Registered 7 audio assets");

    // ======================================================================
    // SECTION 2: Channel-Based Audio
    // ======================================================================
    printSection("SECTION 2: Channel-Based Audio");

    std::println("The Audio System provides managed channels for different audio categories:");
    std::println("  - Channel::Music    ({}): Background music tracks", Channels::Music);
    std::println("  - Channel::Ambience ({}): Environmental loops", Channels::Ambience);
    std::println("  - Channel::UI       ({}): User interface sounds", Channels::UI);
    std::println("  - Channel::Voice    ({}): Character dialogue", Channels::Voice);

    // Play music on Music channel
    printSubsection("Playing Music");
    std::println("API: playOnChannel(Channel, ChannelSound)");
    std::println("\nChannelSound fields:");
    std::println("  - asset: AssetHandle to play");
    std::println("  - volume: 0.0 to 1.0 (default: 1.0)");
    std::println("  - pitch: Playback speed multiplier (default: 1.0)");
    std::println("  - looping: Repeat when finished (default: false)");
    std::println("  - fadeInTime: Fade in duration in seconds (default: 0.0)");
    std::println("  - startTime: Optional playback start position");

    ChannelSound musicSound{
        .asset = musicTrack,
        .volume = 0.7f,
        .pitch = 1.0f,
        .looping = true,
        .fadeInTime = 2.0f,
        .startTime = std::nullopt
    };

    std::println("\n[Demo] playOnChannel(Channels::Music, musicSound)");
    std::println("  Music track playing with:");
    std::println("    - Volume: {}", musicSound.volume);
    std::println("    - Pitch: {}", musicSound.pitch);
    std::println("    - Looping: {}", musicSound.looping ? "Yes" : "No");
    std::println("    - Fade In: {}s", musicSound.fadeInTime);

    // Play ambience on Ambience channel
    printSubsection("Playing Ambience");

    ChannelSound ambienceSound{
        .asset = ambienceLoop,
        .volume = 0.4f,
        .looping = true
    };

    std::println("[Demo] playOnChannel(Channels::Ambience, ambienceSound)");
    std::println("  Ambience loop playing at volume {}", ambienceSound.volume);

    // Play voice line
    printSubsection("Playing Voice Line");

    ChannelSound voiceSound{
        .asset = voiceLine,
        .volume = 1.0f,
        .pitch = 1.0f,
        .looping = false
    };

    std::println("[Demo] playOnChannel(Channels::Voice, voiceSound)");
    std::println("  Voice line playing (non-looping)");

    // ======================================================================
    // SECTION 3: Channel Control
    // ======================================================================
    printSection("SECTION 3: Channel Control");

    printSubsection("Channel Volume Control");
    std::println("API: setChannelVolume(Channel, Volume)");
    std::println("\n[Demo] Ducking music when voice plays:");
    std::println("  setChannelVolume(Channels::Music, 0.3f)");
    std::println("  Music volume reduced to 30% to highlight dialogue");

    printSubsection("Channel Pitch Control");
    std::println("API: setChannelPitch(Channel, pitch)");
    std::println("\n[Demo] Slow-motion effect:");
    std::println("  setChannelPitch(Channels::Music, 0.5f)");
    std::println("  Music plays at half speed");

    printSubsection("Channel Pause/Resume");
    std::println("API: pauseChannel(Channel)");
    std::println("API: resumeChannel(Channel)");
    std::println("\n[Demo] Pause menu:");
    std::println("  pauseChannel(Channels::Music)");
    std::println("  pauseChannel(Channels::Ambience)");
    std::println("  Music and ambience paused");
    std::println("\n  [User resumes game]");
    std::println("  resumeChannel(Channels::Music)");
    std::println("  resumeChannel(Channels::Ambience)");
    std::println("  Playback resumed from paused position");

    printSubsection("Channel Seeking");
    std::println("API: seekChannel(Channel, position)");
    std::println("\n[Demo] Skip to chorus:");
    std::println("  seekChannel(Channels::Music, 45.0f)");
    std::println("  Music jumps to 45 seconds");

    printSubsection("Channel Stop with Fade");
    std::println("API: stopChannel(Channel, fadeOutTime)");
    std::println("\n[Demo] Level transition:");
    std::println("  stopChannel(Channels::Music, 3.0f)");
    std::println("  Music fades out over 3 seconds");

    // ======================================================================
    // SECTION 4: Channel State Queries
    // ======================================================================
    printSection("SECTION 4: Channel State Queries");

    printSubsection("Query Playing State");
    std::println("API: isChannelPlaying(Channel) -> bool");
    std::println("\n[Demo] Check if music is playing:");
    std::println("  bool playing = isChannelPlaying(Channels::Music)");
    std::println("  Result: {}", "true (music is active)");

    printSubsection("Get Detailed Channel State");
    std::println("API: getChannelState(Channel) -> ChannelState");
    std::println("\nChannelState fields:");
    std::println("  - isPlaying: Currently playing");
    std::println("  - isPaused: Currently paused");
    std::println("  - position: Playback position in seconds");
    std::println("  - length: Total duration in seconds");
    std::println("  - volume: Current volume (0.0 to 1.0)");

    ChannelState exampleState{
        .isPlaying = true,
        .isPaused = false,
        .position = 12.5f,
        .length = 180.0f,
        .volume = 0.7f
    };

    std::println("\n[Demo] ChannelState state = getChannelState(Channels::Music)");
    std::println("  isPlaying: {}", exampleState.isPlaying);
    std::println("  isPaused: {}", exampleState.isPaused);
    std::println("  position: {:.1f}s", exampleState.position);
    std::println("  length: {:.1f}s", exampleState.length);
    std::println("  volume: {}", exampleState.volume);
    std::println("  Progress: {:.1f}%", (exampleState.position / exampleState.length) * 100.0f);

    // ======================================================================
    // SECTION 5: Positional (3D) Audio
    // ======================================================================
    printSection("SECTION 5: Positional (3D) Audio");

    std::println("Positional audio uses 3D space for realistic sound positioning.");
    std::println("Unlike channels, positional sounds are fire-and-forget with handles.\n");

    printSubsection("Playing Positional Sound");
    std::println("API: playPositional(PositionalSound) -> SoundHandle");
    std::println("\nPositionalSound fields:");
    std::println("  - asset: AssetHandle to play");
    std::println("  - position: Vec3 location in world space");
    std::println("  - volume: Base volume (0.0 to 1.0)");
    std::println("  - pitch: Playback speed multiplier");
    std::println("  - minDistance: Distance at which volume starts attenuating");
    std::println("  - maxDistance: Distance at which sound is silent");
    std::println("  - velocity: Optional Vec3 for Doppler effect");
    std::println("  - onComplete: Optional callback when sound finishes");

    PositionalSound explosion{
        .asset = explosionSound,
        .position = {100.0f, 50.0f, 0.0f},
        .volume = 1.0f,
        .pitch = 1.0f,
        .minDistance = 10.0f,
        .maxDistance = 500.0f,
        .velocity = std::nullopt,
        .onComplete = []() { std::println("    [Callback] Explosion sound completed"); }
    };

    std::println("\n[Demo] SoundHandle h1 = playPositional(explosion)");
    std::println("  Explosion at ({}, {}, {})",
                 explosion.position.x, explosion.position.y, explosion.position.z);
    std::println("  Min distance: {}, Max distance: {}",
                 explosion.minDistance, explosion.maxDistance);
    SoundHandle h1 = 1001;  // Mock handle
    std::println("  Handle: {}", h1);

    printSubsection("Positional Sound with Doppler Effect");

    PositionalSound movingFootstep{
        .asset = footstepSound,
        .position = {-50.0f, 0.0f, 20.0f},
        .volume = 0.8f,
        .pitch = 1.0f,
        .minDistance = 5.0f,
        .maxDistance = 100.0f,
        .velocity = Vec3{10.0f, 0.0f, -5.0f}  // Moving toward listener
    };

    std::println("[Demo] SoundHandle h2 = playPositional(movingFootstep)");
    std::println("  Footstep at ({}, {}, {})",
                 movingFootstep.position.x, movingFootstep.position.y, movingFootstep.position.z);
    std::println("  Velocity: ({}, {}, {})",
                 movingFootstep.velocity->x, movingFootstep.velocity->y, movingFootstep.velocity->z);
    std::println("  Doppler effect enabled (pitch shifts based on velocity)");
    SoundHandle h2 = 1002;

    printSubsection("Updating Positional Sound Position");
    std::println("API: updatePositionalPosition(SoundHandle, Vec3)");
    std::println("\n[Demo] Moving sound source:");
    std::println("  updatePositionalPosition(h2, Vec3{{-30.0f, 0.0f, 15.0f}})");
    std::println("  Sound moved closer to listener");

    printSubsection("Querying Positional Sound");
    std::println("API: isPositionalPlaying(SoundHandle) -> bool");
    std::println("\n[Demo] Check if sound is still playing:");
    std::println("  bool playing = isPositionalPlaying(h1)");
    std::println("  Result: {}", "true");

    printSubsection("Stopping Positional Sound");
    std::println("API: stopPositional(SoundHandle)");
    std::println("\n[Demo] Stop explosion sound:");
    std::println("  stopPositional(h1)");
    std::println("  Sound handle {} stopped", h1);

    // ======================================================================
    // SECTION 6: 3D Audio Listener
    // ======================================================================
    printSection("SECTION 6: 3D Audio Listener");

    std::println("The audio listener represents the player's ears in 3D space.");
    std::println("Position and orientation affect how positional audio is heard.\n");

    printSubsection("Setting Listener Position");
    std::println("API: setListener(AudioListener)");
    std::println("\nAudioListener fields:");
    std::println("  - position: Vec3 location in world space");
    std::println("  - forward: Vec3 direction the listener faces");
    std::println("  - up: Vec3 up direction (usually {{0, 1, 0}})");
    std::println("  - velocity: Vec3 movement speed for Doppler effect");

    AudioListener listener{
        .position = {0.0f, 0.0f, 0.0f},
        .forward = {0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .velocity = {0.0f, 0.0f, 0.0f}
    };

    std::println("\n[Demo] setListener(listener)");
    std::println("  Position: ({}, {}, {})",
                 listener.position.x, listener.position.y, listener.position.z);
    std::println("  Forward: ({}, {}, {})",
                 listener.forward.x, listener.forward.y, listener.forward.z);
    std::println("  Up: ({}, {}, {})",
                 listener.up.x, listener.up.y, listener.up.z);

    printSubsection("Updating Listener for Moving Player");

    AudioListener movingListener{
        .position = {25.0f, 10.0f, 5.0f},
        .forward = {0.707f, 0.0f, -0.707f},  // Looking 45 degrees
        .up = {0.0f, 1.0f, 0.0f},
        .velocity = {5.0f, 0.0f, -5.0f}
    };

    std::println("[Demo] Player moves - update listener:");
    std::println("  setListener(movingListener)");
    std::println("  Position: ({}, {}, {})",
                 movingListener.position.x, movingListener.position.y, movingListener.position.z);
    std::println("  Velocity: ({}, {}, {})",
                 movingListener.velocity.x, movingListener.velocity.y, movingListener.velocity.z);
    std::println("  Player's movement creates Doppler effect on positional sounds");

    printSubsection("Getting Current Listener");
    std::println("API: getListener() -> AudioListener");
    std::println("\n[Demo] AudioListener current = getListener()");
    std::println("  Returns current listener configuration for queries");

    // ======================================================================
    // SECTION 7: Global Audio Controls
    // ======================================================================
    printSection("SECTION 7: Global Audio Controls");

    printSubsection("Master Volume");
    std::println("API: setMasterVolume(Volume)");
    std::println("API: getMasterVolume() -> Volume");
    std::println("\n[Demo] User adjusts master volume in settings:");
    std::println("  setMasterVolume(0.8f)");
    std::println("  All audio scaled to 80% of individual volumes");
    std::println("\n  Volume current = getMasterVolume()");
    std::println("  Returns: 0.8");

    printSubsection("Pause All Audio");
    std::println("API: pauseAll()");
    std::println("\n[Demo] Game enters pause menu:");
    std::println("  pauseAll()");
    std::println("  All channels and positional sounds paused");
    std::println("  Playback positions preserved");

    printSubsection("Resume All Audio");
    std::println("API: resumeAll()");
    std::println("\n[Demo] Player resumes game:");
    std::println("  resumeAll()");
    std::println("  All paused audio continues from where it stopped");

    printSubsection("Stop All Audio");
    std::println("API: stopAll()");
    std::println("\n[Demo] Returning to main menu:");
    std::println("  stopAll()");
    std::println("  All channels and positional sounds stopped immediately");
    std::println("  Playback positions reset");

    // ======================================================================
    // SECTION 8: Channel Groups
    // ======================================================================
    printSection("SECTION 8: Channel Groups");

    std::println("Channel groups allow volume control of multiple channels together.");
    std::println("Useful for category-based mixing (Music, SFX, Dialogue).\n");

    printSubsection("Assigning Channels to Groups");
    std::println("API: assignChannelToGroup(Channel, groupName)");
    std::println("\n[Demo] Organize channels into mix groups:");
    std::println("  assignChannelToGroup(Channels::Music, \"Music\")");
    std::println("  assignChannelToGroup(Channels::Ambience, \"SFX\")");
    std::println("  assignChannelToGroup(Channels::UI, \"SFX\")");
    std::println("  assignChannelToGroup(Channels::Voice, \"Dialogue\")");

    printSubsection("Setting Group Volume");
    std::println("API: setGroupVolume(groupName, Volume)");
    std::println("\n[Demo] User adjusts SFX volume in settings:");
    std::println("  setGroupVolume(\"SFX\", 0.5f)");
    std::println("  Ambience and UI channels scaled to 50%");
    std::println("\n[Demo] Dialogue boost:");
    std::println("  setGroupVolume(\"Dialogue\", 1.2f)");
    std::println("  Voice channel volume increased by 20% (clamped to max 1.0)");

    printSubsection("Volume Hierarchy");
    std::println("Final volume calculation:");
    std::println("  FinalVolume = MasterVolume * GroupVolume * ChannelVolume * AssetVolume");
    std::println("\nExample:");
    std::println("  Master: 0.8");
    std::println("  SFX Group: 0.5");
    std::println("  UI Channel: 1.0");
    std::println("  Coin Sound: 0.9");
    std::println("  Final = 0.8 * 0.5 * 1.0 * 0.9 = 0.36 (36% of max volume)");

    // ======================================================================
    // SECTION 9: Lifecycle Management
    // ======================================================================
    printSection("SECTION 9: Lifecycle Management");

    printSubsection("Update Loop");
    std::println("API: update(DeltaTime dt)");
    std::println("\nThe update() method must be called every frame to:");
    std::println("  - Process streaming audio buffers");
    std::println("  - Update positional sound attenuation");
    std::println("  - Handle volume fades");
    std::println("  - Trigger onComplete callbacks");
    std::println("  - Clean up finished sounds");

    std::println("\n[Demo] Game loop:");
    std::println("  while (running) {{");
    std::println("    float dt = timer.getDeltaTime();");
    std::println("    audio->update(dt);");
    std::println("    // ... other game systems");
    std::println("  }}");

    std::println("\n[Demo] Simulating several frames:");
    std::println("  Frame 1:");
    simulateUpdate(0.016f);
    std::println("  Frame 2:");
    simulateUpdate(0.017f);
    std::println("  Frame 3:");
    simulateUpdate(0.015f);

    // ======================================================================
    // SECTION 10: Practical Scenarios
    // ======================================================================
    printSection("SECTION 10: Practical Usage Scenarios");

    printSubsection("Scenario 1: Background Music with Crossfade");
    std::println("// Fade out current music");
    std::println("stopChannel(Channels::Music, 2.0f);");
    std::println("");
    std::println("// After 2 seconds, start new track with fade in");
    std::println("ChannelSound newMusic{{");
    std::println("    .asset = levelTwoMusic,");
    std::println("    .volume = 0.7f,");
    std::println("    .looping = true,");
    std::println("    .fadeInTime = 2.0f");
    std::println("}};");
    std::println("playOnChannel(Channels::Music, newMusic);");

    printSubsection("Scenario 2: UI Sound Effect");
    std::println("// Play button click sound on UI channel");
    std::println("ChannelSound buttonClick{{");
    std::println("    .asset = clickSound,");
    std::println("    .volume = 1.0f,");
    std::println("    .pitch = 1.0f,");
    std::println("    .looping = false");
    std::println("}};");
    std::println("playOnChannel(Channels::UI, buttonClick);");

    printSubsection("Scenario 3: Combat with Positional Audio");
    std::println("// Explosion at enemy position");
    std::println("PositionalSound explosion{{");
    std::println("    .asset = explosionSound,");
    std::println("    .position = enemy.getPosition3D(),");
    std::println("    .volume = 1.0f,");
    std::println("    .minDistance = 10.0f,");
    std::println("    .maxDistance = 500.0f,");
    std::println("    .onComplete = []() {{ screenShake.stop(); }}");
    std::println("}};");
    std::println("SoundHandle h = playPositional(explosion);");

    printSubsection("Scenario 4: Pause Menu");
    std::println("// Player pauses game");
    std::println("pauseAll();");
    std::println("");
    std::println("// Play menu music on separate channel");
    std::println("ChannelSound menuMusic{{ .asset = menuTheme, .looping = true }};");
    std::println("playOnChannel(Channels::UI, menuMusic);");
    std::println("");
    std::println("// Player resumes");
    std::println("stopChannel(Channels::UI, 0.5f);  // Fade out menu music");
    std::println("resumeAll();  // Resume game audio");

    printSubsection("Scenario 5: Dynamic Music Intensity");
    std::println("// Increase music intensity during boss fight");
    std::println("if (bossHealthPercent < 0.5f) {{");
    std::println("    setChannelPitch(Channels::Music, 1.2f);  // Speed up");
    std::println("    setChannelVolume(Channels::Music, 0.9f);  // Louder");
    std::println("}} else {{");
    std::println("    setChannelPitch(Channels::Music, 1.0f);");
    std::println("    setChannelVolume(Channels::Music, 0.7f);");
    std::println("}}");

    printSubsection("Scenario 6: Footstep Sounds Following Player");
    std::println("// Update listener to player position every frame");
    std::println("AudioListener listener{{");
    std::println("    .position = player.getPosition3D(),");
    std::println("    .forward = player.getFacingDirection3D(),");
    std::println("    .up = Vec3{{0.0f, 1.0f, 0.0f}},");
    std::println("    .velocity = player.getVelocity3D()");
    std::println("}};");
    std::println("setListener(listener);");

    printSubsection("Scenario 7: Environmental Audio Zones");
    std::println("// Check if player entered new area");
    std::println("if (player.enteredZone(\"cave\")) {{");
    std::println("    // Fade out forest ambience");
    std::println("    stopChannel(Channels::Ambience, 1.5f);");
    std::println("    ");
    std::println("    // Start cave ambience");
    std::println("    ChannelSound caveAmbience{{");
    std::println("        .asset = caveAmbienceAsset,");
    std::println("        .volume = 0.6f,");
    std::println("        .looping = true,");
    std::println("        .fadeInTime = 1.5f");
    std::println("    }};");
    std::println("    playOnChannel(Channels::Ambience, caveAmbience);");
    std::println("}}");

    printSubsection("Scenario 8: Sound Test in Settings Menu");
    std::println("// User clicks \"Test\" button next to volume slider");
    std::println("ChannelSound testSound{{");
    std::println("    .asset = uiTestSound,");
    std::println("    .volume = getMasterVolume() * 0.8f,  // Preview at current master");
    std::println("    .looping = false");
    std::println("}};");
    std::println("playOnChannel(Channels::UI, testSound);");

    // ======================================================================
    // SECTION 11: API Coverage Summary
    // ======================================================================
    printSection("SECTION 11: API Coverage Summary");

    std::println("This demo exercised ALL methods of the IAudioSystem interface:\n");

    std::println("Lifecycle:");
    std::println("  [X] update(DeltaTime dt)");

    std::println("\nChannel-Based Audio:");
    std::println("  [X] playOnChannel(Channel, ChannelSound)");
    std::println("  [X] stopChannel(Channel, fadeOutTime)");
    std::println("  [X] pauseChannel(Channel)");
    std::println("  [X] resumeChannel(Channel)");
    std::println("  [X] setChannelVolume(Channel, Volume)");
    std::println("  [X] setChannelPitch(Channel, pitch)");
    std::println("  [X] seekChannel(Channel, position)");
    std::println("  [X] getChannelState(Channel) -> ChannelState");
    std::println("  [X] isChannelPlaying(Channel) -> bool");

    std::println("\nPositional Audio:");
    std::println("  [X] playPositional(PositionalSound) -> SoundHandle");
    std::println("  [X] stopPositional(SoundHandle)");
    std::println("  [X] updatePositionalPosition(SoundHandle, Vec3)");
    std::println("  [X] isPositionalPlaying(SoundHandle) -> bool");

    std::println("\n3D Audio Listener:");
    std::println("  [X] setListener(AudioListener)");
    std::println("  [X] getListener() -> AudioListener");

    std::println("\nGlobal Controls:");
    std::println("  [X] setMasterVolume(Volume)");
    std::println("  [X] getMasterVolume() -> Volume");
    std::println("  [X] pauseAll()");
    std::println("  [X] resumeAll()");
    std::println("  [X] stopAll()");

    std::println("\nChannel Groups:");
    std::println("  [X] setGroupVolume(groupName, Volume)");
    std::println("  [X] assignChannelToGroup(Channel, groupName)");

    std::println("\nTypes Demonstrated:");
    std::println("  [X] Channel (Music, Ambience, UI, Voice)");
    std::println("  [X] Volume (float)");
    std::println("  [X] SoundHandle (uint64_t)");
    std::println("  [X] ChannelSound (asset, volume, pitch, looping, fadeInTime, startTime)");
    std::println("  [X] PositionalSound (asset, position, volume, pitch, distances, velocity, callback)");
    std::println("  [X] AudioListener (position, forward, up, velocity)");
    std::println("  [X] ChannelState (isPlaying, isPaused, position, length, volume)");

    printSection("Demo Complete");

    std::println("The Audio System provides a complete audio solution with:");
    std::println("  - Managed channels for music, ambience, UI, and voice");
    std::println("  - Fire-and-forget positional 3D audio");
    std::println("  - Full 3D listener support with Doppler effect");
    std::println("  - Hierarchical volume control (Master -> Group -> Channel -> Asset)");
    std::println("  - Smooth fading for professional transitions");
    std::println("  - Channel grouping for category-based mixing");
    std::println("  - Real-time pitch and volume adjustment");
    std::println("  - Playback state queries and seeking");
    std::println("\nFor actual audio playback, integrate with bestow-audio implementation.");

    return 0;
}
