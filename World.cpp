#include "World.h"

#include "Animation.h"

World::World() {
	init();
}

World::~World() {
	PX_RELEASE(scene_);
	PX_RELEASE(dispatcher_);
	PX_RELEASE(physics_);
	PX_RELEASE(foundation_);


}

void World::init() {
	foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, allocator_, errorCallback_);

	physics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation_, physx::PxTolerancesScale(), true, nullptr);

	physx::PxSceneDesc sceneDesc(physics_->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -10.0f ,0.0f);
	dispatcher_ = physx::PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = dispatcher_;
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	scene_ = physics_->createScene(sceneDesc);

	material_ = physics_->createMaterial(0.0f, 0.0f, 0.0f);

	auto plane = PxCreatePlane(*physics_, physx::PxPlane(0, 1, 0, -0.7f), *material_);
	// scene_->addActor(*plane);
}

void World::step(float time) const {
	scene_->simulate(time);
	scene_->fetchResults(true);
}

physx::PxRigidStatic* World::createBoxStatic(physx::PxVec3 pose, float halfX, float halfY, float halfZ) {
	auto shape = physics_->createShape(physx::PxBoxGeometry(halfX, halfY, halfZ), *material_);
	auto body = physics_->createRigidStatic(physx::PxTransform(pose));
	body->attachShape(*shape);
	scene_->addActor(*body);

	shape->release();

	return body;
}

physx::PxRigidDynamic* World::createBoxDynamic(physx::PxVec3 pose, float halfX, float halfY, float halfZ) {
	auto shape = physics_->createShape(physx::PxBoxGeometry(halfX, halfY, halfZ), *material_);
	auto body = physics_->createRigidDynamic(physx::PxTransform(pose));
	body->attachShape(*shape);
	physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	scene_->addActor(*body);

	shape->release();

	return body;
}
