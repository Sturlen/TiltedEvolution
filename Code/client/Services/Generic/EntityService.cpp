#include <TiltedOnlinePCH.h>
#include <Services/EntityService.h>

#include <Events/ReferenceAddedEvent.h>
#include <Events/ReferenceRemovedEvent.h>
#include <Events/ReferenceSpawnedEvent.h>

#include <World.h>

#include <Components.h>

#include <Forms/TESForm.h>

#include <Games/References.h>

EntityService::EntityService(World& aWorld, entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_dispatcher(aDispatcher)
{
    m_referenceAddedConnection = m_dispatcher.sink<ReferenceAddedEvent>().connect<&EntityService::OnReferenceAdded>(this);
    m_referenceRemovedConnection = m_dispatcher.sink<ReferenceRemovedEvent>().connect<&EntityService::OnReferenceRemoved>(this);
}

void EntityService::OnReferenceAdded(const ReferenceAddedEvent& acEvent) noexcept
{
    if (acEvent.FormType == FormType::Character)
    {
        if (acEvent.FormId == 0x14)
        {
            Actor* pActor = RTTI_CAST(TESForm::GetById(acEvent.FormId), TESForm, Actor);
            pActor->GetExtension()->SetPlayer(true);
        }

        entt::entity entity;

        // TODO: why would a reference already have a remote component?
        const auto view = m_world.view<RemoteComponent>();
        const auto it = std::find_if(std::begin(view), std::end(view), [&acEvent, view](entt::entity entity) {
            auto& remoteComponent = view.get<RemoteComponent>(entity);
            return remoteComponent.CachedRefId == acEvent.FormId;
        });

        if (it != std::end(view))
        {
            Actor* pActor = RTTI_CAST(TESForm::GetById(acEvent.FormId), TESForm, Actor);
            pActor->GetExtension()->SetRemote(true);

            entity = *it;
        }
        else
            entity = m_world.create();

        m_world.emplace<FormIdComponent>(entity, acEvent.FormId);

        m_refIdToEntity[acEvent.FormId] = entity;

        m_dispatcher.trigger(ReferenceSpawnedEvent(acEvent.FormId, acEvent.FormType, entity));
    }
}

void EntityService::OnReferenceRemoved(const ReferenceRemovedEvent& acEvent) noexcept
{
    const auto cIterator = m_refIdToEntity.find(acEvent.FormId);

    if (cIterator != std::end(m_refIdToEntity))
    {
        const auto entity = cIterator->second;

        if (m_world.all_of<FormIdComponent>(entity))
            m_world.remove<FormIdComponent>(entity);

        if (m_world.orphan(entity))
            m_world.destroy(entity);

        m_refIdToEntity.erase(cIterator);
    }
}

Outcome<entt::entity, bool> EntityService::GetCharacter(const uint32_t acFormId) const noexcept
{
    const auto cPair = m_refIdToEntity.find(acFormId);
    if (cPair != std::end(m_refIdToEntity))
    {
        const auto entity = cPair->second;

        return entity;
    }

    return false;
}
