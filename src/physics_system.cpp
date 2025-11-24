#include "physics_system.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

bool PhysicsSystem::Initialize()
{
    if (m_physics != nullptr)
    {
        return true;
    }

    m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
    if (!m_foundation)
    {
        return false;
    }

    physx::PxTolerancesScale scale;
    m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, true, nullptr);
    if (!m_physics)
    {
        Shutdown();
        return false;
    }

    if (!PxInitExtensions(*m_physics, nullptr))
    {
        Shutdown();
        return false;
    }

    m_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
    if (!m_dispatcher)
    {
        Shutdown();
        return false;
    }

    physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = m_dispatcher;
    sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
    sceneDesc.simulationEventCallback = this;
    m_pxScene = m_physics->createScene(sceneDesc);
    if (!m_pxScene)
    {
        Shutdown();
        return false;
    }

    m_defaultMaterial = m_physics->createMaterial(0.6f, 0.6f, 0.1f);
    if (!m_defaultMaterial)
    {
        Shutdown();
        return false;
    }

    m_accumulator = 0.0f;
    return true;
}

void PhysicsSystem::Shutdown()
{
    ClearActors();
    ClearMaterials();

    if (m_defaultMaterial)
    {
        m_defaultMaterial->release();
        m_defaultMaterial = nullptr;
    }

    if (m_pxScene)
    {
        m_pxScene->release();
        m_pxScene = nullptr;
    }

    if (m_dispatcher)
    {
        m_dispatcher->release();
        m_dispatcher = nullptr;
    }

    if (m_physics)
    {
        PxCloseExtensions();
        m_physics->release();
        m_physics = nullptr;
    }

    if (m_foundation)
    {
        m_foundation->release();
        m_foundation = nullptr;
    }
}

bool PhysicsSystem::BuildFromScene(Scene& scene)
{
    if (!m_physics || !m_pxScene)
    {
        return false;
    }

    ClearActors();
    ClearMaterials();
    m_containers.clear();

    bool success = true;
    for (SceneObject& object : scene.GetMutableObjects())
    {
        if (!object.HasPhysicsDefinition())
        {
            continue;
        }

        const SceneObjectPhysics& definition = object.GetPhysicsDefinition();
        if (!definition.enabled)
        {
            continue;
        }

        if (definition.mode == PhysicsBodyMode::Container)
        {
            ContainerConstraint constraint;
            constraint.object = &object;
            constraint.definition = definition;
            constraint.position = object.Transform().position;
            constraint.rotation = glm::normalize(glm::quat(glm::radians(object.Transform().rotation)));
            m_containers.push_back(constraint);
            continue;
        }

        if (!CreateRigidActor(object, definition))
        {
            success = false;
        }
    }

    if (m_debugDrawEnabled)
    {
        RefreshDebugData();
    }
    else
    {
        m_debugVertices.clear();
    }

    return success;
}

void PhysicsSystem::Simulate(float deltaTime, Scene& /*scene*/)
{
    if (!m_pxScene)
    {
        return;
    }

    const float clampedDelta = std::clamp(deltaTime, 0.0f, 0.25f);
    m_accumulator += clampedDelta;

    const int maxSteps = 8;
    int steps = 0;

    while (m_accumulator >= m_fixedDelta && steps < maxSteps)
    {
        m_pxScene->simulate(m_fixedDelta);
        m_pxScene->fetchResults(true);
        ApplyContainerConstraints();
        m_accumulator -= m_fixedDelta;
        ++steps;
    }

    if (steps > 0)
    {
        UpdateSceneObjects();
    }

    if (m_debugDrawEnabled)
    {
        RefreshDebugData();
    }
    else if (!m_debugVertices.empty())
    {
        m_debugVertices.clear();
    }
}

void PhysicsSystem::SetDebugRenderingEnabled(bool enabled)
{
    if (m_debugDrawEnabled == enabled)
    {
        return;
    }
    m_debugDrawEnabled = enabled;
    if (m_debugDrawEnabled)
    {
        RefreshDebugData();
    }
    else
    {
        m_debugVertices.clear();
    }
}

void PhysicsSystem::ClearActors()
{
    for (auto& binding : m_bindings)
    {
        if (binding.actor)
        {
            binding.actor->release();
            binding.actor = nullptr;
        }
    }
    m_bindings.clear();
    m_containers.clear();
    m_debugVertices.clear();
}

void PhysicsSystem::ClearMaterials()
{
    for (auto* material : m_ownedMaterials)
    {
        if (material)
        {
            material->release();
        }
    }
    m_ownedMaterials.clear();
}

physx::PxMaterial* PhysicsSystem::CreateMaterial(float friction, float restitution)
{
    if (!m_physics)
    {
        return nullptr;
    }
    const float clampedFriction = std::max(friction, 0.0f);
    const float clampedRestitution = std::clamp(restitution, 0.0f, 1.0f);
    physx::PxMaterial* material = m_physics->createMaterial(clampedFriction, clampedFriction, clampedRestitution);
    if (material)
    {
        m_ownedMaterials.push_back(material);
    }
    return material;
}

physx::PxTransform PhysicsSystem::BuildActorTransform(const SceneObject& object) const
{
    const SceneObjectTransform& transform = object.Transform();
    glm::mat4 rotationMatrix(1.0f);
    rotationMatrix = glm::rotate(rotationMatrix,
                                 glm::radians(transform.rotation.x),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix,
                                 glm::radians(transform.rotation.y),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix,
                                 glm::radians(transform.rotation.z),
                                 glm::vec3(0.0f, 0.0f, 1.0f));
    glm::quat rotation = glm::normalize(glm::quat_cast(rotationMatrix));
    return physx::PxTransform(ToPx(transform.position), ToPx(rotation));
}

bool PhysicsSystem::CreateRigidActor(SceneObject& object, const SceneObjectPhysics& definition)
{
    physx::PxRigidActor* actor = nullptr;
    const bool isDynamic = definition.mass > 0.0f;
    const physx::PxTransform transform = BuildActorTransform(object);

    if (isDynamic)
    {
        actor = m_physics->createRigidDynamic(transform);
    }
    else
    {
        actor = m_physics->createRigidStatic(transform);
    }

    if (!actor)
    {
        return false;
    }

    physx::PxMaterial* material = CreateMaterial(definition.friction, definition.restitution);
    if (!material)
    {
        actor->release();
        return false;
    }

    glm::vec3 localOffset{ 0.0f };
    physx::PxShape* shape = CreateShapeForDefinition(*actor, object, definition, *material, localOffset);
    if (!shape)
    {
        actor->release();
        return false;
    }

    m_pxScene->addActor(*actor);

    ActorBinding binding;
    binding.object = &object;
    binding.definition = definition;
    binding.actor = actor;
    binding.isDynamic = isDynamic;
    binding.localOffset = localOffset;
    m_bindings.push_back(binding);

    if (isDynamic)
    {
        if (auto* dynamicBody = actor->is<physx::PxRigidDynamic>())
        {
            const float mass = std::max(definition.mass, 0.01f);
            physx::PxRigidBodyExt::updateMassAndInertia(*dynamicBody, mass);
            dynamicBody->setLinearDamping(definition.linearDamping);
            dynamicBody->setAngularDamping(definition.angularDamping);
            dynamicBody->setLinearVelocity(ToPx(definition.initialVelocity));
        }
    }

    return true;
}

physx::PxShape* PhysicsSystem::CreateShapeForDefinition(physx::PxRigidActor& actor,
                                                        const SceneObject& object,
                                                        const SceneObjectPhysics& definition,
                                                        physx::PxMaterial& material,
                                                        glm::vec3& outLocalOffset)
{
    physx::PxShape* shape = nullptr;
    if (definition.shape == PhysicsShapeType::Sphere)
    {
        physx::PxSphereGeometry geometry(definition.radius);
        shape = physx::PxRigidActorExt::createExclusiveShape(actor, geometry, material);
    }
    else
    {
        const physx::PxVec3 halfExtents = ToPx(glm::max(definition.halfExtents, glm::vec3(0.05f)));
        physx::PxBoxGeometry geometry(halfExtents);
        shape = physx::PxRigidActorExt::createExclusiveShape(actor, geometry, material);
    }

    if (!shape)
    {
        return nullptr;
    }

    outLocalOffset = glm::vec3(0.0f);
    if (object.HasBounds() && definition.alignToBounds)
    {
        const glm::vec3 localCenter = object.GetLocalBoundsCenter();
        const SceneObjectTransform& transform = object.Transform();
        outLocalOffset = glm::vec3(localCenter.x * transform.scale.x,
                                   localCenter.y * transform.scale.y,
                                   localCenter.z * transform.scale.z);
        shape->setLocalPose(physx::PxTransform(ToPx(outLocalOffset)));
    }

    shape->setContactOffset(0.02f);
    shape->setRestOffset(0.0f);
    return shape;
}

void PhysicsSystem::UpdateSceneObjects()
{
    for (auto& binding : m_bindings)
    {
        if (!binding.isDynamic || binding.actor == nullptr || binding.object == nullptr)
        {
            continue;
        }

        const physx::PxTransform pose = binding.actor->getGlobalPose();
        const glm::quat rotation = ToGlm(pose.q);
        const glm::vec3 center = ToGlm(pose.p);
        const glm::vec3 position = center - rotation * binding.localOffset;
        binding.object->ApplyPhysicsPose(position, rotation);
    }
}

void PhysicsSystem::ApplyContainerConstraints()
{
    if (m_containers.empty())
    {
        return;
    }

    for (auto& binding : m_bindings)
    {
        if (!binding.isDynamic || binding.actor == nullptr)
        {
            continue;
        }

        for (const auto& container : m_containers)
        {
            ApplyContainerConstraint(container, binding);
        }
    }
}

void PhysicsSystem::ApplyContainerConstraint(const ContainerConstraint& container, ActorBinding& binding)
{
    auto* dynamicBody = binding.actor->is<physx::PxRigidDynamic>();
    if (!dynamicBody)
    {
        return;
    }

    physx::PxTransform pose = dynamicBody->getGlobalPose();
    glm::vec3 position = ToGlm(pose.p);
    glm::vec3 velocity = ToGlm(dynamicBody->getLinearVelocity());
    glm::vec3 angularVelocity = ToGlm(dynamicBody->getAngularVelocity());
    bool modified = false;

    if (container.definition.shape == PhysicsShapeType::Sphere)
    {
        const glm::vec3 toCenter = position - container.position;
        const float distance = glm::length(toCenter);
        const float dynamicRadius = (binding.definition.shape == PhysicsShapeType::Sphere)
                                        ? binding.definition.radius
                                        : glm::length(binding.definition.halfExtents);
        const float maxDistance = container.definition.radius - dynamicRadius;
        if (maxDistance > 0.0f && distance > maxDistance)
        {
            glm::vec3 normal = glm::normalize(toCenter);
            if (glm::length(normal) <= std::numeric_limits<float>::epsilon())
            {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            position = container.position + normal * maxDistance;
            const float velAlongNormal = glm::dot(velocity, normal);
            if (velAlongNormal > 0.0f)
            {
                velocity -= (1.0f + binding.definition.restitution) * velAlongNormal * normal;
            }
            
            if (binding.definition.shape == PhysicsShapeType::Sphere && dynamicRadius > 0.0f)
            {
                glm::vec3 tangentVelocity = velocity - glm::dot(velocity, normal) * normal;
                const float tangentSpeed = glm::length(tangentVelocity);
                if (tangentSpeed > 0.001f)
                {
                    glm::vec3 tangentDir = glm::normalize(tangentVelocity);
                    glm::vec3 rotationAxis = glm::cross(tangentDir, normal);
                    if (glm::length(rotationAxis) > 0.001f)
                    {
                        rotationAxis = glm::normalize(rotationAxis);
                        const float targetAngularSpeed = tangentSpeed / dynamicRadius;
                        angularVelocity = rotationAxis * targetAngularSpeed;
                    }
                }
            }
            else if (binding.definition.shape == PhysicsShapeType::Box)
            {
                glm::vec3 tangentVelocity = velocity - glm::dot(velocity, normal) * normal;
                const float tangentSpeed = glm::length(tangentVelocity);
                if (tangentSpeed > 0.001f)
                {
                    glm::vec3 tangentDir = glm::normalize(tangentVelocity);
                    glm::vec3 rotationAxis = glm::cross(tangentDir, normal);
                    if (glm::length(rotationAxis) > 0.001f)
                    {
                        rotationAxis = glm::normalize(rotationAxis);
                        const float avgHalfExtent = (binding.definition.halfExtents.x + 
                                                     binding.definition.halfExtents.y + 
                                                     binding.definition.halfExtents.z) / 3.0f;
                        const float effectiveRadius = avgHalfExtent * 1.2f;
                        const float targetAngularSpeed = tangentSpeed / effectiveRadius;
                        angularVelocity = rotationAxis * targetAngularSpeed;
                    }
                }
            }
            
            modified = true;
        }
    }
    else
    {
        glm::vec3 localPosition = glm::conjugate(container.rotation) * (position - container.position);
        glm::vec3 localVelocity = glm::conjugate(container.rotation) * velocity;

        glm::vec3 dynamicHalfExtents = binding.definition.shape == PhysicsShapeType::Sphere
            ? glm::vec3(binding.definition.radius)
            : binding.definition.halfExtents;
        dynamicHalfExtents = glm::max(dynamicHalfExtents, glm::vec3(0.01f));

        glm::vec3 minBounds = -container.definition.halfExtents + dynamicHalfExtents;
        glm::vec3 maxBounds = container.definition.halfExtents - dynamicHalfExtents;
        minBounds = glm::min(minBounds, glm::vec3(0.0f));
        maxBounds = glm::max(maxBounds, glm::vec3(0.0f));

        glm::vec3 clamped = localPosition;
        glm::vec3 adjustedVelocity = localVelocity;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (clamped[axis] < minBounds[axis])
            {
                clamped[axis] = minBounds[axis];
                adjustedVelocity[axis] = -adjustedVelocity[axis] * binding.definition.restitution;
                modified = true;
            }
            else if (clamped[axis] > maxBounds[axis])
            {
                clamped[axis] = maxBounds[axis];
                adjustedVelocity[axis] = -adjustedVelocity[axis] * binding.definition.restitution;
                modified = true;
            }
        }

        if (modified)
        {
            position = container.position + container.rotation * clamped;
            velocity = container.rotation * adjustedVelocity;
        }
    }

    if (modified)
    {
        pose.p = ToPx(position);
        dynamicBody->setGlobalPose(pose);
        dynamicBody->setLinearVelocity(ToPx(velocity));
        if (binding.definition.shape == PhysicsShapeType::Sphere || binding.definition.shape == PhysicsShapeType::Box)
        {
            dynamicBody->setAngularVelocity(ToPx(angularVelocity));
        }
    }
}

void PhysicsSystem::RefreshDebugData()
{
    if (!m_debugDrawEnabled)
    {
        m_debugVertices.clear();
        return;
    }

    m_debugVertices.clear();
    m_debugVertices.reserve((m_bindings.size() + m_containers.size()) * 96);

    for (const auto& binding : m_bindings)
    {
        if (binding.actor == nullptr)
        {
            continue;
        }
        BuildActorDebugGeometry(binding);
    }

    for (const auto& constraint : m_containers)
    {
        BuildContainerDebugGeometry(constraint);
    }
}

void PhysicsSystem::BuildActorDebugGeometry(const ActorBinding& binding)
{
    if (binding.actor == nullptr)
    {
        return;
    }

    const physx::PxTransform pose = binding.actor->getGlobalPose();
    glm::quat rotation = ToGlm(pose.q);
    glm::vec3 center = ToGlm(pose.p) + rotation * binding.localOffset;

    const glm::vec3 dynamicColor(0.2f, 0.95f, 0.2f);
    const glm::vec3 staticColor(0.95f, 0.9f, 0.25f);
    const glm::vec3 color = binding.isDynamic ? dynamicColor : staticColor;

    if (binding.definition.shape == PhysicsShapeType::Sphere)
    {
        BuildSphereDebugGeometry(center, rotation, binding.definition.radius, color);
    }
    else
    {
        BuildBoxDebugGeometry(center, rotation, binding.definition.halfExtents, color);
    }
}

void PhysicsSystem::BuildContainerDebugGeometry(const ContainerConstraint& constraint)
{
    const glm::vec3 color(0.92f, 0.35f, 0.35f);
    if (constraint.definition.shape == PhysicsShapeType::Sphere)
    {
        BuildSphereDebugGeometry(constraint.position, constraint.rotation, constraint.definition.radius, color);
    }
    else
    {
        BuildBoxDebugGeometry(constraint.position, constraint.rotation, constraint.definition.halfExtents, color);
    }
}

void PhysicsSystem::BuildSphereDebugGeometry(const glm::vec3& center,
                                             const glm::quat& rotation,
                                             float radius,
                                             const glm::vec3& color)
{
    const glm::vec3 axisX = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 axisY = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 axisZ = rotation * glm::vec3(0.0f, 0.0f, 1.0f);

    AddCircle(center, axisX, axisY, radius, color);
    AddCircle(center, axisX, axisZ, radius, color);
    AddCircle(center, axisY, axisZ, radius, color);
}

void PhysicsSystem::BuildBoxDebugGeometry(const glm::vec3& center,
                                          const glm::quat& rotation,
                                          const glm::vec3& halfExtents,
                                          const glm::vec3& color)
{
    const glm::vec3 axisX = rotation * glm::vec3(halfExtents.x, 0.0f, 0.0f);
    const glm::vec3 axisY = rotation * glm::vec3(0.0f, halfExtents.y, 0.0f);
    const glm::vec3 axisZ = rotation * glm::vec3(0.0f, 0.0f, halfExtents.z);

    std::array<glm::vec3, 8> corners{};
    int index = 0;
    for (int sx : { -1, 1 })
    {
        for (int sy : { -1, 1 })
        {
            for (int sz : { -1, 1 })
            {
                corners[index++] = center + axisX * static_cast<float>(sx)
                                             + axisY * static_cast<float>(sy)
                                             + axisZ * static_cast<float>(sz);
            }
        }
    }

    static constexpr int edges[12][2] = {
        {0,1},{0,2},{0,4},
        {1,3},{1,5},
        {2,3},{2,6},
        {3,7},
        {4,5},{4,6},
        {5,7},
        {6,7}
    };

    for (const auto& edge : edges)
    {
        AddDebugLine(corners[edge[0]], corners[edge[1]], color);
    }
}

void PhysicsSystem::AddCircle(const glm::vec3& center,
                              const glm::vec3& axisA,
                              const glm::vec3& axisB,
                              float radius,
                              const glm::vec3& color)
{
    const int segments = 48;
    for (int i = 0; i < segments; ++i)
    {
        const float t0 = (static_cast<float>(i) / static_cast<float>(segments)) * glm::two_pi<float>();
        const float t1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * glm::two_pi<float>();
        const glm::vec3 p0 = center + (axisA * std::cos(t0) + axisB * std::sin(t0)) * radius;
        const glm::vec3 p1 = center + (axisA * std::cos(t1) + axisB * std::sin(t1)) * radius;
        AddDebugLine(p0, p1, color);
    }
}

void PhysicsSystem::AddDebugLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color)
{
    m_debugVertices.push_back({ a, color });
    m_debugVertices.push_back({ b, color });
}

physx::PxVec3 PhysicsSystem::ToPx(const glm::vec3& value)
{
    return physx::PxVec3(value.x, value.y, value.z);
}

physx::PxQuat PhysicsSystem::ToPx(const glm::quat& value)
{
    return physx::PxQuat(value.x, value.y, value.z, value.w);
}

glm::vec3 PhysicsSystem::ToGlm(const physx::PxVec3& value)
{
    return glm::vec3(value.x, value.y, value.z);
}

glm::quat PhysicsSystem::ToGlm(const physx::PxQuat& value)
{
    return glm::normalize(glm::quat(value.w, value.x, value.y, value.z));
}


