from music_agent.schemas import (
    ApprovalRequestMessage,
    FinalResultMessage,
    PlanPreviewMessage,
    ProgressMessage,
)


def test_plan_preview_message_matches_phase_two_protocol():
    message = PlanPreviewMessage(
        planId="plan-1",
        sessionId="session-1",
        summary="创建学习歌单并添加最近常听歌曲",
        riskLevel="medium",
        steps=[
            {"stepId": "step-1", "title": "创建歌单 学习歌单"},
            {"stepId": "step-2", "title": "获取最近常听歌曲", "status": "pending"},
        ],
    )

    assert message.model_dump(by_alias=True) == {
        "type": "plan_preview",
        "planId": "plan-1",
        "sessionId": "session-1",
        "summary": "创建学习歌单并添加最近常听歌曲",
        "riskLevel": "medium",
        "steps": [
            {"stepId": "step-1", "title": "创建歌单 学习歌单", "status": None},
            {"stepId": "step-2", "title": "获取最近常听歌曲", "status": "pending"},
        ],
    }


def test_approval_request_message_includes_session_and_risk_level():
    message = ApprovalRequestMessage(
        planId="plan-1",
        sessionId="session-1",
        message="即将创建歌单并批量添加 20 首歌曲，是否继续？",
        riskLevel="high",
    )

    assert message.model_dump(by_alias=True) == {
        "type": "approval_request",
        "planId": "plan-1",
        "sessionId": "session-1",
        "message": "即将创建歌单并批量添加 20 首歌曲，是否继续？",
        "riskLevel": "high",
    }


def test_progress_message_matches_protocol():
    message = ProgressMessage(
        planId="plan-1",
        stepId="step-2",
        message="已创建歌单，正在获取最近常听歌曲",
    )

    assert message.model_dump(by_alias=True) == {
        "type": "progress",
        "planId": "plan-1",
        "stepId": "step-2",
        "message": "已创建歌单，正在获取最近常听歌曲",
    }


def test_final_result_message_matches_protocol():
    message = FinalResultMessage(
        planId="plan-1",
        sessionId="session-1",
        ok=True,
        summary="已创建歌单“学习歌单”，并添加 20 首歌曲",
    )

    assert message.model_dump(by_alias=True) == {
        "type": "final_result",
        "planId": "plan-1",
        "sessionId": "session-1",
        "ok": True,
        "summary": "已创建歌单“学习歌单”，并添加 20 首歌曲",
    }
