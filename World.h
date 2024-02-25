#pragma once

#include <physx/PxPhysicsAPI.h>

class World {
public:
	World();
	~World();

	void step(float time) const;

	physx::PxRigidStatic* createBoxStatic(physx::PxVec3 pose, float halfX, float halfY, float halfZ);
	physx::PxRigidDynamic* createBoxDynamic(physx::PxVec3 pose, float halfX, float halfY, float halfZ);
	// physx::PxPlane* createPlane(physx::PxVec3)

	physx::PxDefaultAllocator allocator_;
	physx::PxDefaultErrorCallback errorCallback_;
	physx::PxFoundation* foundation_;
	physx::PxPhysics* physics_;
	physx::PxDefaultCpuDispatcher* dispatcher_;
	physx::PxScene* scene_;
	physx::PxMaterial* material_;

private:
	void init();	
};
