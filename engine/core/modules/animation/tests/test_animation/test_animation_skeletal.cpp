// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/animation/components/animation_component.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/animation/components/skeleton_socket_component.h"
#include "nau/animation/data/events.h"
#include "nau/animation/playback/animation_transforms.h"
#include "nau/animation/controller/animation_controller.h"
#include "nau/animation/controller/animation_controller_blend.h"
#include "nau/scene/components/skinned_mesh_component.h"
#include "nau/messaging/messaging.h"
#include "nau/scene/scene_object.h"
#include "nau/io/virtual_file_system.h"
#include "scene_test_base.h"

#include <filesystem>

namespace nau::test
{
    class TestAnimationSkeletal : public SceneTestBase
    {
    public:
        async::Task<> skipAnimFrames(animation::AnimationController& controller, int frameCount)
        {
            const int startFrame = controller.getCurrentFrame();
            const int targetFrame = startFrame + frameCount;

            while (targetFrame > controller.getCurrentFrame())
            {
                co_await skipFrames(1);
            }
        }
    private:
        void initializeApp() override
        {
            configureVirtualFileSystem(getServiceProvider().get<io::IVirtualFileSystem>());
        }

        async::Task<> loadSkeletalMeshAsset()
        {
            using namespace nau::scene;

            AssetRef<> sceneAssetRef{"file:/content/scenes/yarumy/yarumy.gltf"};

            SceneAsset::Ptr sceneAsset = co_await sceneAssetRef.getAssetViewTyped<SceneAsset>();

            IScene::Ptr scene = getServiceProvider().get<scene::ISceneFactory>().createSceneFromAsset(*sceneAsset);
            
            co_await getSceneManager().activateScene(std::move(scene));
        }

        void configureVirtualFileSystem(io::IVirtualFileSystem& vfs)
        {
            namespace fs = std::filesystem;

            const auto projectContentDir = EXPR_Block->fs::path
            {
                const fs::path contentRelativePath{L"engine/core/modules/animation/tests/test_animation/content"};
                fs::path currentPath = fs::current_path();

                do
                {
                    auto targetPath = currentPath / contentRelativePath;
                    if (fs::exists(targetPath))
                    {
                        return fs::canonical(targetPath);
                    }

                    currentPath = currentPath.parent_path();

                } while (currentPath.has_relative_path());

                return {};
            };

            auto contentFs = io::createNativeFileSystem(projectContentDir.string());
            vfs.mount("/content", std::move(contentFs)).ignore();
        }
    };

    static nau::math::vec3 GetBoneModelPosition(SkeletonComponent& skeletonComponent, unsigned boneIndex)
    {
        const ozz::math::Float4x4& boneModelMatrix = skeletonComponent.getModelSpaceJointMatrices()[boneIndex];
        alignas(16) float translation[4]{};
        ozz::math::Store3Ptr(boneModelMatrix.cols[3], translation);
        const math::vec3 bonePosition = math::vec3(translation[0], translation[1], translation[2]);

        return bonePosition;
    }

    static nau::math::vec3 GetBoneWorldPosition(SkeletonComponent& skeletonComponent, unsigned boneIndex)
    {
        const ozz::math::Float4x4& boneModelMatrix = skeletonComponent.getModelSpaceJointMatrices()[boneIndex];
        math::Matrix4 modelM;
        std::memcpy(static_cast<void*>(&modelM), &boneModelMatrix, sizeof(modelM));
        const math::Matrix4& modelTr = skeletonComponent.getWorldTransform().getMatrix();

        return (modelTr*modelM).getTranslation();
    }

    TEST_F(TestAnimationSkeletal, SkinnedMeshWithSkeletonLoadAndPlayback)
    {
        using namespace nau::animation;
        using namespace nau::async;
        using namespace nau::math;
        using namespace nau::scene;
        using namespace testing;

        const AssertionResult testResult = runTestApp([&]() -> Task<AssertionResult>
            {
                AssetRef<> sceneAssetRef{"file:/content/scenes/yarumy/yarumy.gltf"};

                SceneAsset::Ptr sceneAsset = co_await sceneAssetRef.getAssetViewTyped<SceneAsset>();

                IScene::Ptr scene = getServiceProvider().get<scene::ISceneFactory>().createSceneFromAsset(*sceneAsset);

                SkeletonComponent* skeletonComponent = nullptr;
                SkinnedMeshComponent* skinnedMeshComponent = nullptr;
                AnimationComponent* animationComponent = nullptr;

                for (SceneObject* const obj : scene->getRoot().getChildObjects(true))
                {
                    if (obj->getName().find("YarumaBody", 0) == 0)
                    {
                        skeletonComponent = obj->findFirstComponent<SkeletonComponent>();
                        skinnedMeshComponent = obj->findFirstComponent<SkinnedMeshComponent>();
                        animationComponent = obj->findFirstComponent<AnimationComponent>();

                        if (!skeletonComponent)
                        {
                            co_return AssertionFailure() << "SkeletonComponent is not added when loading SkinnedMesh with Skeleton (animated)";
                        }
                        if (!skinnedMeshComponent)
                        {
                            co_return AssertionFailure() << "SkinnedMeshComponent is not added when loading SkinnedMesh with Skeleton (animated)";
                        }
                        if (!animationComponent)
                        {
                            co_return AssertionFailure() << "AnimationComponent is not added when loading SkinnedMesh with Skeleton (animated)";
                        }
                    }
                }

                ObjectWeakRef sceneRef = co_await getSceneManager().activateScene(std::move(scene));

                unsigned bonesCount = skeletonComponent->getBonesCount();
                ASSERT_ASYNC(bonesCount == 19);

                const math::vec3 lArmStartPos = GetBoneModelPosition(*skeletonComponent, 5);

                co_await skipFrames(30);

                const math::vec3 armLoweLPosWalked = GetBoneModelPosition(*skeletonComponent, 5);

                ASSERT_ASYNC(armLoweLPosWalked != lArmStartPos); // animation should change Bone position 

                AnimationController* animController = animationComponent->getController();

                skeletonComponent->setSkeletonToDefaultPose();

                const math::vec3 lArmDefaultPos = GetBoneModelPosition(*skeletonComponent, 5);

                // setSkeletonToDefaultPose should set Bone position to initial state (initial should be DefaultPose, not checked in test)
                ASSERT_ASYNC(lArmStartPos == lArmDefaultPos); 

                 //Stop all animations
                for (int i = 0; i < animController->getAnimationInstancesCount(); ++i)
                {
                    AnimationInstance* animInstance = animController->getAnimationInstanceAt(i);
                    animInstance->getPlayer()->stop();
                }

                co_await skipFrames(30);

                const math::vec3 lArmPausedPos = GetBoneModelPosition(*skeletonComponent, 5);

                ASSERT_ASYNC(lengthSqr(lArmPausedPos - lArmDefaultPos) < MATH_SMALL_NUMBER);

                co_return AssertionSuccess();
            });

        ASSERT_TRUE(testResult);
    }

        TEST_F(TestAnimationSkeletal, SkeletonSocketComponent)
        {
            using namespace nau::animation;
            using namespace nau::async;
            using namespace nau::math;
            using namespace nau::scene;
            using namespace testing;

            const AssertionResult testResult = runTestApp([&]() -> Task<AssertionResult>
                {
                    AssetRef<> sceneAssetRef{"file:/content/scenes/yarumy/yarumy.gltf"};

                    SceneAsset::Ptr sceneAsset = co_await sceneAssetRef.getAssetViewTyped<SceneAsset>();

                    IScene::Ptr scene = getServiceProvider().get<scene::ISceneFactory>().createSceneFromAsset(*sceneAsset);

                    SceneObject* skeletonSocketObject = nullptr;
                    SkeletonComponent* skeletonComponent = nullptr;
                    for (SceneObject* const obj : scene->getRoot().getChildObjects(true))
                    {
                        if (obj->getName().find("YarumaBody", 0) == 0)
                        {
                            skeletonComponent = obj->findFirstComponent<SkeletonComponent>();

                            auto& sceneFactory = getServiceProvider().get<ISceneFactory>();
                            scene::ObjectUniquePtr<SceneObject> skeletonSocket = sceneFactory.createSceneObject<SkeletonSocketComponent>();
                            skeletonSocketObject = skeletonSocket.get();

                            skeletonSocket->setName("SkeletonSocket_ArmLeft");

                            auto& skeletonSocketComponent = skeletonSocket->getRootComponent<SkeletonSocketComponent>();
                            skeletonSocketComponent.setBoneName("armLowe.L");
                            skeletonSocketComponent.setRelativeToBoneOffset(math::Transform(math::quat::identity(), math::Vector3(0, 0, 0)));

                            obj->attachChild(std::move(skeletonSocket));
                        }
                    }

                    ObjectWeakRef sceneRef = co_await getSceneManager().activateScene(std::move(scene));

                    // first frame socketItem is in (0,0,0), will become valid after first update

                    co_await skipFrames(10);

                    const math::vec3 lArmUpdatedPos = GetBoneModelPosition(*skeletonComponent, 5);
                    const math::vec3 socketUpdatedPos = skeletonSocketObject->getTransform().getTranslation();

                    const bool isSocketCloseEnoughToTargetBoneModelPos = length(lArmUpdatedPos - socketUpdatedPos) < 0.01f;
                    ASSERT_ASYNC(isSocketCloseEnoughToTargetBoneModelPos);

                    const math::vec3 lArmUpdatedWPos = GetBoneWorldPosition(*skeletonComponent, 5);
                    const math::vec3 socketUpdatedWPos = skeletonSocketObject->getWorldTransform().getTranslation();

                    const bool isSocketCloseEnoughToTargetBoneWorldPos = length(lArmUpdatedWPos - socketUpdatedWPos) < 0.01f;
                    ASSERT_ASYNC(isSocketCloseEnoughToTargetBoneWorldPos);

                    co_return AssertionSuccess();
                });

            ASSERT_TRUE(testResult);
        }

}  // namespace nau::test
