#pragma once

#include <vector>

#include <PxPhysicsAPI.h>

#include <glm/glm.hpp>

#include "scene.h"

struct PhysicsDebugVertex
{
    glm::vec3 position{ 0.0f };
    glm::vec3 color{ 0.0f };
};

class PhysicsSystem : public physx::PxSimulationEventCallback
{
public:
    PhysicsSystem() = default;
    ~PhysicsSystem();

    bool Initialize();
    void Shutdown();

    bool BuildFromScene(Scene& scene);
    void Simulate(float deltaTime, Scene& scene);
    void SetDebugRenderingEnabled(bool enabled);
    bool IsDebugRenderingEnabled() const { return m_debugDrawEnabled; }
    const std::vector<PhysicsDebugVertex>& GetDebugVertices() const { return m_debugVertices; }

    // PxSimulationEventCallback interface
    void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override {}
    void onWake(physx::PxActor**, physx::PxU32) override {}
    void onSleep(physx::PxActor**, physx::PxU32) override {}
    void onContact(const physx::PxContactPairHeader&, const physx::PxContactPair*, physx::PxU32) override {}
    void onTrigger(physx::PxTriggerPair*, physx::PxU32) override {}
    void onAdvance(const physx::PxRigidBody*const*, const physx::PxTransform*, physx::PxU32) override {}

private:
    struct ActorBinding
    {
        SceneObject* object = nullptr;
        SceneObjectPhysics definition{};
        physx::PxRigidActor* actor = nullptr;
        bool isDynamic = false;
        glm::vec3 localOffset{ 0.0f };
    };

    struct ContainerConstraint
    {
        SceneObject* object = nullptr;
        SceneObjectPhysics definition{};
        glm::vec3 position{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    };

    void ClearActors();
    void ClearMaterials();
    physx::PxMaterial* CreateMaterial(float friction, float restitution);
    physx::PxTransform BuildActorTransform(const SceneObject& object) const;
    bool CreateRigidActor(SceneObject& object, const SceneObjectPhysics& definition);
    physx::PxShape* CreateShapeForDefinition(physx::PxRigidActor& actor,
                                             const SceneObject& object,
                                             const SceneObjectPhysics& definition,
                                             physx::PxMaterial& material,
                                             glm::vec3& outLocalOffset);
    void UpdateSceneObjects();
    void ApplyContainerConstraints();
    void ApplyContainerConstraint(const ContainerConstraint& container, ActorBinding& binding);
    void RefreshDebugData();
    void BuildActorDebugGeometry(const ActorBinding& binding);
    void BuildContainerDebugGeometry(const ContainerConstraint& constraint);
    void BuildSphereDebugGeometry(const glm::vec3& center,
                                  const glm::quat& rotation,
                                  float radius,
                                  const glm::vec3& color);
    void BuildBoxDebugGeometry(const glm::vec3& center,
                               const glm::quat& rotation,
                               const glm::vec3& halfExtents,
                               const glm::vec3& color);
    void AddCircle(const glm::vec3& center,
                   const glm::vec3& axisA,
                   const glm::vec3& axisB,
                   float radius,
                   const glm::vec3& color);
    void AddDebugLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color);
    static physx::PxVec3 ToPx(const glm::vec3& value);
    static physx::PxQuat ToPx(const glm::quat& value);
    static glm::vec3 ToGlm(const physx::PxVec3& value);
    static glm::quat ToGlm(const physx::PxQuat& value);

    physx::PxDefaultAllocator m_allocator;
    physx::PxDefaultErrorCallback m_errorCallback;
    physx::PxFoundation* m_foundation = nullptr;
    physx::PxPhysics* m_physics = nullptr;
    physx::PxScene* m_pxScene = nullptr;
    physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
    physx::PxMaterial* m_defaultMaterial = nullptr;
    std::vector<physx::PxMaterial*> m_ownedMaterials;
    std::vector<ActorBinding> m_bindings;
    std::vector<ContainerConstraint> m_containers;
    std::vector<PhysicsDebugVertex> m_debugVertices;
    bool m_debugDrawEnabled = false;
    float m_accumulator = 0.0f;
    const float m_fixedDelta = 1.0f / 120.0f;
};


