// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "animation_manager_ui_controller.h"

#include <imgui.h>

#include <algorithm>
#include <string>
#include <vector>

#include "nau/animation/animation_manager.h"
#include "nau/animation/components/animation_component.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/animation/controller/animation_controller.h"
#include "nau/animation/playback/animation_instance.h"
#include "nau/memory/stack_allocator.h"
#include "nau/scene/scene.h"
#include "nau/scene/scene_object.h"
#include "nau/string/string_conv.h"

namespace nau::animation
{
    namespace
    {
        const char* getPlayModeString(PlayMode playMode)
        {
            switch (playMode)
            {
                case PlayMode::Once:
                    return "Once";
                case PlayMode::Looping:
                    return "Looping";
                case PlayMode::PingPong:
                    return "PingPong";
                default:
                    return "Unknown";
            }
        }

        const char* getInterpolationMethodString(AnimationInterpolationMethod method)
        {
            switch (method)
            {
                case AnimationInterpolationMethod::Step:
                    return "Step";
                case AnimationInterpolationMethod::Linear:
                    return "Linear";
                default:
                    return "Unknown";
            }
        }

        nau::string formatComponentTitle(const auto& components, int index)
        {
            if (0 <= index && index < components.size())
            {
                const auto* component = components[index];
                const std::string_view parentObjectName = strings::toStringView(component->getParentObject().getName());
                const std::string_view componentName = strings::toStringView(component->getName());

                return ::fmt::format("{}:{}", parentObjectName, componentName);
            }
            return "-- select component --";
        }

        const char* formatAnimInstanceTitle(const AnimationInstance* animInstance)
        {
            if (animInstance)
            {
                return reinterpret_cast<const char*>(animInstance->getName().c_str());
            }
            return "-- select animation --";
        }

        void drawAnimInstanceUi(AnimationController* controller, int targetTrackIndex)
        {
            auto* targetAnimInstance = controller->getAnimationInstanceAt(targetTrackIndex);

            if (targetAnimInstance == nullptr)
            {
                return;
            }

            PlayMode currentPlayMode = targetAnimInstance->getPlayMode();

            if (ImGui::BeginCombo("Play mode", getPlayModeString(currentPlayMode)))
            {
                for (int modeIndex = 0; modeIndex <= (int)PlayMode::PingPong; ++modeIndex)
                {
                    if (ImGui::Selectable(getPlayModeString((PlayMode)modeIndex), modeIndex == (int)currentPlayMode))
                    {
                        targetAnimInstance->setPlayMode((PlayMode)modeIndex);
                        targetAnimInstance->setIsReversed(false);
                    }
                }

                ImGui::EndCombo();
            }

            AnimationInterpolationMethod currentInterpMethod = targetAnimInstance->getInterpolationMethod();

            if (ImGui::BeginCombo("Interpolation method", getInterpolationMethodString(currentInterpMethod)))
            {
                for (int methodIndex = 0; methodIndex <= (int)AnimationInterpolationMethod::Linear; ++methodIndex)
                {
                    if (ImGui::Selectable(getInterpolationMethodString((AnimationInterpolationMethod)methodIndex), methodIndex == (int)currentInterpMethod))
                    {
                        targetAnimInstance->setInterpolationMethod((AnimationInterpolationMethod)methodIndex);
                    }
                }

                ImGui::EndCombo();
            }

            bool isReversed = targetAnimInstance->isReversed();

            if (ImGui::Checkbox("Reverse", &isReversed))
            {
                targetAnimInstance->setIsReversed(isReversed);
            }

            if (ImGui::Button("Play"))
            {
                targetAnimInstance->getPlayer()->play();
            }
            ImGui::SameLine();
            if (ImGui::Button(targetAnimInstance->getPlayer()->isPaused() ? "Unpause" : "Pause"))
            {
                targetAnimInstance->getPlayer()->pause(!targetAnimInstance->getPlayer()->isPaused());
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                targetAnimInstance->getPlayer()->stop();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset"))
            {
                targetAnimInstance->debugIgnoreController(false);
            }
            float userWeight = targetAnimInstance->getWeight();
            if (ImGui::SliderFloat("Weight", &userWeight, 0.0f, 1.0f))
            {
                targetAnimInstance->debugIgnoreController(true);
                targetAnimInstance->setWeight(userWeight);
            }
            float playbackSpeed = targetAnimInstance->getPlayer()->getPlaybackSpeed();
            if (ImGui::SliderFloat("Speed", &playbackSpeed, 0.0f, 4.0f))
            {
                targetAnimInstance->getPlayer()->setPlaybackSpeed(playbackSpeed);
            }
            {
                const int currentFrame = targetAnimInstance->getCurrentFrame();
                const int durationFrames = targetAnimInstance->getPlayer()->getDurationInFrames();
                float timePos = currentFrame / (float)durationFrames;
                if (ImGui::SliderFloat("Timeline", &timePos, 0.0f, 1.f))
                {
                    int frameToSet = static_cast<int>(timePos * durationFrames);
                    targetAnimInstance->getPlayer()->jumpToFrame(frameToSet);
                }
            }
        }

        void drawAnimComponentUi(const AnimationComponent& animComponent, int& selectedTrackIndex)
        {
            if (auto* controller = animComponent.getController())
            {
                if (ImGui::BeginCombo("Loaded animations", formatAnimInstanceTitle(controller->getAnimationInstanceAt(selectedTrackIndex))))
                {
                    for (int trackIndex = 0; trackIndex < controller->getAnimationInstancesCount(); ++trackIndex)
                    {
                        const char* selectedAnimName = formatAnimInstanceTitle(controller->getAnimationInstanceAt(trackIndex));
                        if (ImGui::Selectable(selectedAnimName, trackIndex == selectedTrackIndex))
                        {
                            selectedTrackIndex = trackIndex;
                        }
                    }

                    ImGui::EndCombo();
                }

                drawAnimInstanceUi(controller, selectedTrackIndex);

                if (ImGui::Button("Stop all"))
                {
                    for (int trackIndex = 0; trackIndex < controller->getAnimationInstancesCount(); ++trackIndex)
                    {
                        if (auto* animInstance = controller->getAnimationInstanceAt(trackIndex))
                        {
                            animInstance->getPlayer()->stop();
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset weights"))
                {
                    for (int trackIndex = 0; trackIndex < controller->getAnimationInstancesCount(); ++trackIndex)
                    {
                        if (auto* animInstance = controller->getAnimationInstanceAt(trackIndex))
                        {
                            animInstance->debugIgnoreController(false);
                        }
                    }
                }
            }
        }

        void drawSkeletonComponentUi(SkeletonComponent& skelComp)
        {
            if (ImGui::Button("Default pose"))
            {
                skelComp.setSkeletonToDefaultPose();

                if (auto* animComponent = skelComp.getParentObject().findFirstComponent<AnimationComponent>())
                {
                    if (auto* controller = animComponent->getController())
                    {
                        for (int trackIndex = 0; trackIndex < controller->getAnimationInstancesCount(); ++trackIndex)
                        {
                            if (auto* animInstance = controller->getAnimationInstanceAt(trackIndex))
                            {
                                animInstance->getPlayer()->stop();
                            }
                        }
                    }
                }
            }
        }
    }  // namespace

    AnimationManagerImguiController::AnimationManagerImguiController(AnimationManager& owner)
    {
        if (const auto* scene = owner.getParentObject().getScene())
        {
            m_name = std::string{scene->getName()};

            if (m_name.empty())
            {
                m_name = std::string{toString(scene->getUid()).c_str()};
            }
        }
    }

    void AnimationManagerImguiController::drawGui(const std::vector<scene::ObjectWeakRef<AnimationComponent>>& animComponents)
    {
        if (ImGui::Begin("Animation system"))
        {
            ImGui::SetWindowPos({ 200, 100 }, ImGuiCond_Once);
            ImGui::SetWindowSize(ImVec2(400, 200), ImGuiCond_Once);

            if (ImGui::CollapsingHeader(fmt::format("Scene [{}]", m_name).c_str()))
            {
                LocalStackAllocator allocator;

                if (!animComponents.empty() && ImGui::CollapsingHeader("Objects control"))
                {
                    std::vector<const AnimationComponent*> existingComponents;

                    for (auto& animComponentRef : animComponents)
                    {
                        if (animComponentRef)
                        {
                            existingComponents.push_back(animComponentRef.get());
                        }
                    }

                    if (!existingComponents.empty())
                    {
                        if (ImGui::Button("Stop all"))
                        {
                            for (const AnimationComponent* animComponent : existingComponents)
                            {
                                if (AnimationController* controller = animComponent->getController())
                                {
                                    for (int trackIndex = 0; trackIndex < controller->getAnimationInstancesCount(); ++trackIndex)
                                    {
                                        if (auto* animInstance = controller->getAnimationInstanceAt(trackIndex))
                                        {
                                            animInstance->getPlayer()->stop();
                                        }
                                    }
                                }
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset all"))
                        {
                            for (const AnimationComponent* animComponent : existingComponents)
                            {
                                if (AnimationController* controller = animComponent->getController())
                                {
                                    for (int trackIndex = 0; trackIndex < controller->getAnimationInstancesCount(); ++trackIndex)
                                    {
                                        if (auto* animInstance = controller->getAnimationInstanceAt(trackIndex))
                                        {
                                            animInstance->debugIgnoreController(false);
                                        }
                                    }
                                }
                            }
                        }

                        if (ImGui::BeginCombo("Objects", formatComponentTitle(existingComponents, m_selectedAnimComponentIndex).tostring().c_str()))
                        {
                            for (int componentIndex = 0; componentIndex < existingComponents.size(); ++componentIndex)
                            {
                                if (ImGui::Selectable(formatComponentTitle(existingComponents, componentIndex).tostring().c_str(), componentIndex == m_selectedAnimComponentIndex))
                                {
                                    m_selectedAnimComponentIndex = componentIndex;
                                    m_selectedTrackIndex = -1;
                                }
                            }

                            ImGui::EndCombo();
                        }
                    }

                    if (m_selectedAnimComponentIndex != -1)
                    {
                        const AnimationComponent& animComponent = *existingComponents[m_selectedAnimComponentIndex];

                        drawAnimComponentUi(animComponent, m_selectedTrackIndex);
                    }
                }

                if (ImGui::CollapsingHeader("Skeleton"))
                {
                    std::vector<SkeletonComponent*> existingAnimatedSkeletons;

                    for (auto& animComponentRef : animComponents)
                    {
                        if (animComponentRef)
                        {
                            if (auto* skelComp = animComponentRef->getParentObject().findFirstComponent<SkeletonComponent>())
                            {
                                existingAnimatedSkeletons.push_back(skelComp);
                            }
                        }
                    }

                    ImGui::Checkbox("Debug Skeleton", &SkeletonComponent::drawDebugSkeletons);

                    if (ImGui::BeginCombo("Skeletons", formatComponentTitle(existingAnimatedSkeletons, m_selectedSkeletonComponentIndex).tostring().c_str()))
                    {
                        for (int componentIndex = 0; componentIndex < existingAnimatedSkeletons.size(); ++componentIndex)
                        {
                            if (ImGui::Selectable(formatComponentTitle(existingAnimatedSkeletons, componentIndex).tostring().c_str(), componentIndex == m_selectedSkeletonComponentIndex))
                            {
                                m_selectedSkeletonComponentIndex = componentIndex;
                            }
                        }

                        ImGui::EndCombo();
                    }

                    if (m_selectedSkeletonComponentIndex != -1)
                    {
                        SkeletonComponent& skelComp = *existingAnimatedSkeletons[m_selectedSkeletonComponentIndex];

                        drawSkeletonComponentUi(skelComp);
                    }
                }
            }

            ImGui::End();
        }
    }

}  // namespace nau::animation