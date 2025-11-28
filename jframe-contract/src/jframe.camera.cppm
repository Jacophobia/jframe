// jframe-contract/src/jframe.camera.cppm
// Camera system interface for JFrame

module;

export module jframe.camera;

import jframe.types;
import std;

export namespace jframe {

class ICameraSystem {
public:
    virtual ~ICameraSystem() = default;

    // Target following
    virtual void setTarget(Entity target) = 0;
    virtual void clearTarget() = 0;
    virtual Entity getTarget() const = 0;

    // Follow behavior
    virtual void setFollowSmoothing(float smoothing) = 0;  // 0.0 = instant, 1.0 = very slow
    virtual void setOffset(Vec2 offset) = 0;  // Offset from target position
    virtual void setDeadzone(Vec2 size) = 0;  // Area where camera doesn't move

    // Bounds
    virtual void setBounds(float minX, float maxX, float minY, float maxY) = 0;
    virtual void clearBounds() = 0;

    // Effects
    virtual void shake(float intensity, float duration) = 0;
    virtual void stopShake() = 0;

    // Zoom
    virtual void setZoom(float zoom) = 0;
    virtual float getZoom() const = 0;

    // Update and state
    virtual void update(DeltaTime dt, Vec2 targetPosition) = 0;
    virtual Camera getCamera() const = 0;
    virtual Vec2 getPosition() const = 0;

    // Coordinate conversion
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;
    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;
};

}  // namespace jframe
