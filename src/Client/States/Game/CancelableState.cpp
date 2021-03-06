// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Client/States/Game/CancelableState.hpp>
#include <Client/States/BackgroundState.hpp>
#include <Client/States/MainMenuState.hpp>
#include <NDK/StateMachine.hpp>

namespace bw
{
	CancelableState::CancelableState(std::shared_ptr<StateData> stateData, std::shared_ptr<AbstractState> originalState) :
	StatusState(std::move(stateData)),
	m_originalState(std::move(originalState))
	{
		m_cancelButton = CreateWidget<Ndk::ButtonWidget>();
		m_cancelButton->UpdateText(Nz::SimpleTextDrawer::Draw("Cancel", 24));
		m_cancelButton->Resize(m_cancelButton->GetPreferredSize());

		m_cancelButton->OnButtonTrigger.Connect([this](const Ndk::ButtonWidget*)
		{
			Cancel();
		});
	}

	void CancelableState::Cancel(float delay)
	{
		UpdateState([this](Ndk::StateMachine& fsm)
		{
			fsm.ResetState(std::make_shared<BackgroundState>(GetStateDataPtr()));
			fsm.PushState(std::move(m_originalState));
		}, delay);

		OnCancelled();
	}

	void CancelableState::SwitchToState(std::shared_ptr<AbstractState> state, float delay)
	{
		UpdateState([this, state = std::move(state)](Ndk::StateMachine& fsm) mutable
		{
			fsm.ChangeState(std::move(state));
		}, delay);
	}

	void CancelableState::UpdateState(std::function<void(Ndk::StateMachine& fsm)> stateUpdate, float delay)
	{
		m_nextStateCallback = std::move(stateUpdate);
		m_nextStateDelay = delay;
	}

	void CancelableState::LayoutWidgets()
	{
		Nz::Vector2f canvasSize = GetStateData().canvas->GetSize();
		m_cancelButton->SetPosition(canvasSize.x / 2.f - m_cancelButton->GetWidth() / 2.f, canvasSize.y - 10.f - m_cancelButton->GetHeight());
	}

	bool CancelableState::Update(Ndk::StateMachine& fsm, float elapsedTime)
	{
		if (!StatusState::Update(fsm, elapsedTime))
			return false;

		if (m_nextStateCallback)
		{
			if ((m_nextStateDelay -= elapsedTime) < 0.f)
			{
				m_nextStateCallback(fsm);
				return true;
			}
		}

		return true;
	}
}
