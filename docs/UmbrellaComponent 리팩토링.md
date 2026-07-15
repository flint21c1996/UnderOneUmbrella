# UmbrellaComponent 리팩토링

## 질문

> 획득/제거 상태 변경을 SetState()로 일원화해 이벤트 중복 호출 제거  
> 우산 제거 중 불필요한 물방울 생성 방지  
> 물 붓기 상태에서 뒤집힌 우산 비주얼이 잘못 표시되던 문제 수정  
> 월드 디버그 드로잉이 디버그 설정을 따르도록 수정  
> 닫기·열기·뒤집기·붓기 상태 전이 테스트 추가
>
> 각2자 뭘한건지 자세히 말해봐

## 답변

이번 변경은 아직 클래스를 분리한 단계가 아니라, 분리하기 전에 기존 동작을 안정시키고 명백한 상태 불일치를 수정한 1차 작업입니다.

### 1. 획득/제거 상태 변경을 `SetState()`로 일원화

관련 코드: `Source/UnderOneUmBrella/Player/UOUUmbrellaComponent.cpp`, `Source/UnderOneUmBrella/Player/UOUUmbrellaComponent.h`

기존 `AcquireUmbrella()`와 `RemoveUmbrella()`는 다음 두 가지 방법을 섞어 사용했습니다.

```cpp
SetState(EUOUUmbrellaState::Closed);
OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
```

문제는 `SetState()` 자체도 실제 상태가 변경되면 마지막에 이벤트를 발생시킨다는 점입니다.

```cpp
RefreshVisuals();
UpdatePouringEffectState();
OnUmbrellaStateChanged.Broadcast(CurrentState, bHasUmbrella);
```

따라서 기존 제거 흐름은 우산이 `Open`, `UpsideDown`, `Pouring` 상태일 때:

```text
RemoveUmbrella
 → SetState(Closed)
   → OnUmbrellaStateChanged 이벤트
 → RemoveUmbrella에서 직접 이벤트
```

처럼 동일한 제거 동작에서 이벤트가 두 번 전달될 수 있었습니다. UI, 애니메이션 블루프린트, 사운드 등 이벤트 구독자가 있다면 제거 처리를 두 번 실행할 위험이 있습니다.

변경 후에는 직접 `Broadcast()`하지 않고 다음과 같이 호출합니다.

```cpp
SetState(EUOUUmbrellaState::Closed, true);
```

`SetState()`에는 다음 선택지가 추가됐습니다.

```cpp
void SetState(
    EUOUUmbrellaState NewState,
    bool bBroadcastIfUnchanged = false);
```

의미는 다음과 같습니다.

- 상태가 실제로 달라지면 기존처럼 이벤트를 한 번 발생
- 상태가 같아도 소유 여부처럼 다른 정보가 바뀌었다면 `true`를 통해 이벤트를 한 번 발생
- 호출하는 쪽에서는 직접 `Broadcast()`하지 않음

획득 상황에서는 우산 상태가 원래부터 `Closed`일 가능성이 높습니다. 하지만 `bHasUmbrella`는 `false → true`로 바뀌므로 외부에 알릴 필요가 있습니다.

```cpp
bHasUmbrella = true;
SetState(EUOUUmbrellaState::Closed, true);
```

제거 상황도 상태가 이미 `Closed`여도 `bHasUmbrella`가 `true → false`로 바뀌므로 이벤트가 필요합니다.

```cpp
bHasUmbrella = false;
SetState(EUOUUmbrellaState::Closed, true);
```

즉, 상태와 소유권 변경 알림의 순서를 `SetState()` 한 군데가 관리하도록 만든 것입니다.

다만 `SetClosedReversedVisualOverride()`의 이벤트는 순수 비주얼 오버라이드 알림이라 아직 별도로 남아 있습니다. 이번 변경은 획득/제거 경로를 정리한 것이지, 모든 이벤트를 완전히 통합한 단계는 아닙니다.

---

### 2. 제거 중 불필요한 물방울 생성 방지

관련 코드: `Source/UnderOneUmBrella/Player/UOUUmbrellaComponent.cpp`

`SetState()`에는 `Pouring` 상태에서 빠져나갈 때 마지막으로 모아둔 물을 물방울 액터로 만드는 기능이 있습니다.

```cpp
if (PreviousState == EUOUUmbrellaState::Pouring
    && ResolvedState != EUOUUmbrellaState::Pouring)
{
    if (!SpawnPendingPourDrop())
    {
        ResetPendingPourDrop();
    }
}
```

이 동작은 정상적인 `EndPour()`에서는 필요합니다.

```text
물을 붓다가 입력을 놓음
 → Pouring에서 UpsideDown으로 전환
 → 마지막 프레임에 모인 물을 물방울로 생성
```

하지만 기존 `RemoveUmbrella()`도 바로 `SetState(Closed)`를 호출했습니다.

```text
물을 붓는 중 우산 제거
 → Pouring에서 Closed로 전환
 → SetState가 마지막 물방울 생성
 → 그 뒤 저장된 물 초기화
```

즉, 우산을 버리거나 제거하는 상황인데 물방울 액터가 한 번 더 생성될 수 있었습니다.

변경 후 제거 과정에서는 먼저 보류 중인 물방울 데이터를 초기화합니다.

```cpp
bHasUmbrella = false;
CurrentDirectionState = Normal;
ResetPendingPourDrop();
```

`ResetPendingPourDrop()`은 다음 값을 비웁니다.

```cpp
PendingPourDropVolume = 0.0f;
PendingPourDropDuration = 0.0f;
TimeSinceLastPourDropSpawn = 0.0f;
```

그다음 저장된 물도 비우고 마지막에 상태를 닫힘으로 전환합니다.

```cpp
StoredWaterContainer->SetAmount(0.0f);
SetState(EUOUUmbrellaState::Closed, true);
```

`SetState()`가 `SpawnPendingPourDrop()`을 호출하더라도 이미 부피와 시간이 0이기 때문에 액터가 생성되지 않습니다.

이 변경으로 종료 원인이 구분됩니다.

- `EndPour()` → 남은 물방울을 정상적으로 생성
- `RemoveUmbrella()` → 보류 중인 물을 폐기하고 생성하지 않음

---

### 3. 붓는 동안 뒤집힌 우산 비주얼 수정

관련 코드: `Source/UnderOneUmBrella/Player/UOUUmbrellaComponent.cpp`

현재 우산에는 두 종류의 상태가 있습니다.

- 논리 상태 `EUOUUmbrellaState`
  - `Closed`
  - `Open`
  - `UpsideDown`
  - `Pouring`
- 표시 상태 `EUOUUmbrellaVisualState`
  - `Closed`
  - `Open`
  - `OpenReversed`

`Pouring`은 게임플레이 상태지만 외형상으로는 우산을 뒤집은 모습이어야 합니다.

```text
UmbrellaState::Pouring
 → UmbrellaVisualState::OpenReversed
```

기존 정적 메시 표시 로직은 표시 상태가 아니라 논리 상태를 직접 검사했습니다.

```cpp
OpenVisual->SetVisibility(
    CurrentState == Open
    || (CurrentState == Pouring && !bUseRuntimeUpsideDownFallback));

UpsideDownVisual->SetVisibility(
    CurrentState == UpsideDown);
```

전용 `UpsideDownVisual` 메시가 있는 상태에서 물을 붓기 시작하면:

```text
CurrentState = Pouring
CurrentVisualState = OpenReversed
bUseRuntimeUpsideDownFallback = false
```

따라서 기존 결과는:

```text
OpenVisual        = 표시
UpsideDownVisual  = 숨김
```

이었습니다. 논리적으로는 뒤집힌 상태인데 일반적으로 펼친 우산 메시가 보인 것입니다.

변경 후에는 `CurrentVisualState`를 기준으로 메시를 선택합니다.

```cpp
UpsideDownVisual->SetVisibility(
    CurrentVisualState == EUOUUmbrellaVisualState::OpenReversed,
    true);
```

일반 펼침 메시도 표시 상태에 따라 결정합니다.

```cpp
OpenVisual->SetVisibility(
    CurrentVisualState == EUOUUmbrellaVisualState::Open
    || (
        CurrentVisualState == EUOUUmbrellaVisualState::OpenReversed
        && UpsideDownVisual == nullptr
        && !bUseRuntimeUpsideDownFallback
    ),
    true);
```

결과는 다음과 같습니다.

| 상황 | 표시 결과 |
|---|---|
| 일반 펼침 | `OpenVisual` |
| 뒤집힘 | `UpsideDownVisual` |
| 물 붓기 | `UpsideDownVisual` |
| 뒤집힘 전용 메시 없음 | `RuntimeHeldVisual`을 회전하거나 `OpenVisual`로 대체 |

핵심은 게임플레이 상태를 비주얼 코드가 직접 해석하지 않고, 이미 계산된 `CurrentVisualState`만 바라보도록 바꾼 것입니다. 이후 비주얼 책임을 별도 클래스로 분리할 때도 이 구조가 더 유리합니다.

---

### 4. 월드 디버그 드로잉에 전역 디버그 설정 적용

관련 코드: `Source/UnderOneUmBrella/Player/UOUUmbrellaComponent.cpp`

이 함수는 Tick에서 계속 호출됩니다.

기존에는 컴포넌트 내부 설정만 확인했습니다.

```cpp
if (!bHasUmbrella
    || (!bDrawPourSocketDebug && !bDrawPourDropSpawnDebug))
{
    return;
}
```

따라서 컴포넌트의 디버그 옵션이 켜져 있으면 프로젝트의 전체 디버그 월드 드로잉을 꺼도 다음 요소가 계속 표시될 수 있었습니다.

- 물 붓기 소켓 위치 구체
- 소켓 좌표축
- 물방울 생성 위치
- 물방울 발사 방향
- 디버그 문자열

변경 후에는 프로젝트 디버그 서브시스템의 `Player` 카테고리도 확인합니다.

```cpp
if (!UUOUDebugSubsystem::IsDebugWorldDrawEnabled(
        this,
        EUOUDebugCategory::Player))
{
    return;
}
```

현재 표시 조건은 두 단계를 모두 통과해야 합니다.

```text
프로젝트 Player 디버그 카테고리 활성화
 AND
컴포넌트의 PourSocket/DropSpawn 디버그 옵션 활성화
```

즉, 로컬 옵션은 “어떤 우산 디버그를 그릴 것인가”를 결정하고, 디버그 서브시스템은 “현재 월드에서 Player 디버그를 허용할 것인가”를 결정합니다.

---

### 5. 상태 전이 자동화 테스트 추가

관련 코드: `Source/UnderOneUmBrella/Tests/UOUUmbrellaComponentTests.cpp`

두 개의 UE Automation Test를 추가했습니다.

#### `StateTransitions`

다음 순서로 실제 public API를 호출합니다.

```text
우산 획득
 → 펼치기
 → 뒤집기
 → 닫기
 → 제거
```

각 단계에서 다음을 확인합니다.

```cpp
Umbrella->AcquireUmbrella();
TestTrue(HasUmbrella());
TestTrue(IsClosed());
TestEqual(GetCurrentVisualState(), Closed);
```

```cpp
Umbrella->OpenUmbrella();
TestTrue(IsOpen());
TestEqual(GetCurrentVisualState(), Open);
```

```cpp
Umbrella->TurnUmbrellaUpsideDown();
TestTrue(IsUpsideDown());
TestEqual(GetCurrentVisualState(), OpenReversed);
```

```cpp
Umbrella->CloseUmbrella();
TestTrue(IsClosed());
TestTrue(IsNormalDirection());
```

```cpp
Umbrella->RemoveUmbrella();
TestFalse(HasUmbrella());
```

앞으로 컴포넌트를 여러 클래스로 분리하더라도 외부에서 보는 기본 동작이 변하지 않았는지 검증하는 안전장치입니다.

#### `PourVisualState`

실제 물 컨테이너를 연결하고 다음 과정을 검사합니다.

```text
우산 획득
 → 뒤집기
 → 물 1.0 저장
 → 붓기 시작
 → 붓기 종료
```

붓기 시작 조건과 표시 상태를 확인합니다.

```cpp
TestTrue(Umbrella->IsPouring());

TestEqual(
    Umbrella->GetCurrentVisualState(),
    EUOUUmbrellaVisualState::OpenReversed);
```

붓기가 끝나면 다시 뒤집힌 상태로 돌아가는지도 확인합니다.

```cpp
Umbrella->EndPour();
TestTrue(Umbrella->IsUpsideDown());
```

두 테스트 모두 UE 5.7 자동화 테스트에서 성공했습니다.

현재 테스트의 한계도 있습니다. 아직 아래 항목은 직접 검증하지 않습니다.

- 이벤트가 정확히 한 번 발생했는지
- 제거 중 물방울 액터가 생성되지 않았는지
- 실제 `UStaticMeshComponent`의 가시성
- 디버그 드로잉 호출 여부

현재 테스트는 상태 머신과 계산된 비주얼 상태를 고정한 1차 안전장치입니다. 다음 단계에서 이벤트 수신 테스트와 실제 컴포넌트/월드를 사용하는 통합 테스트를 추가하는 것이 좋습니다.

---

## 2차 리팩토링 질문

> 그리고 다음 리팩토링 진행하자.
>
> 그것도 아까처럼 어떤작업을 했는지 적고 md에 저장해봐

## 2차 리팩토링 답변: 비주얼 정책 분리

이번 작업에서는 `UUOUUmbrellaComponent`가 직접 담당하던 비주얼 선택 규칙을 `FUOUUmbrellaVisualPolicy`라는 별도 순수 정책 클래스로 분리했습니다.

이번에도 블루프린트와 레벨에 저장된 기존 에셋 설정을 보호하기 위해 `ClosedVisual`, `OpenVisual`, `UpsideDownVisual`, 소켓 이름, 오프셋, 애니메이션 등의 `UPROPERTY`는 이동하지 않았습니다. 직렬화된 데이터를 옮기는 큰 변경 전에, 데이터에 의존하는 판단 로직부터 안전하게 분리한 단계입니다.

### 1. 기존에는 어떤 책임이 섞여 있었는가

기존 `RefreshVisuals()`는 한 함수 안에서 다음 작업을 모두 처리했습니다.

1. 게임플레이 상태를 비주얼 상태로 변환
2. 스켈레탈 우산을 사용할지 정적 메시를 사용할지 선택
3. 뒤집힌 전용 메시가 있는지 검사
4. 전용 메시가 없을 때 런타임 복사 메시를 뒤집을지 결정
5. 런타임 메시도 없으면 일반 펼침 메시를 대체재로 사용할지 결정
6. 각 메시 컴포넌트의 실제 가시성을 변경
7. 런타임 메시의 Transform을 변경

예를 들어 뒤집힌 우산을 표시하는 판단이 다음과 같이 컴포넌트 내부에 직접 들어 있었습니다.

```cpp
const bool bUseRuntimeUpsideDownFallback = RuntimeHeldVisual != nullptr
    && UpsideDownVisual == nullptr
    && ShouldFlipRuntimeHeldVisual();
```

그리고 같은 함수 안에서 다시 `CurrentVisualState`, `UpsideDownVisual`, 런타임 대체 여부를 조합해 각각의 표시 여부를 계산했습니다.

```cpp
OpenVisual->SetVisibility(
    CurrentVisualState == EUOUUmbrellaVisualState::Open
    || (CurrentVisualState == EUOUUmbrellaVisualState::OpenReversed
        && UpsideDownVisual == nullptr
        && !bUseRuntimeUpsideDownFallback),
    true);
```

이 구조에서는 비주얼 우선순위를 바꿀 때마다 거대한 컴포넌트를 수정해야 합니다. 실제 메시 컴포넌트를 생성하지 않으면 표시 규칙만 따로 테스트하기도 어렵습니다.

### 2. `FUOUUmbrellaVisualPolicy` 추가

새 파일을 추가했습니다.

- `Source/UnderOneUmBrella/Player/UOUUmbrellaVisualPolicy.h`
- `Source/UnderOneUmBrella/Player/UOUUmbrellaVisualPolicy.cpp`

이 클래스는 `UObject`나 `UActorComponent`가 아닌 일반 C++ 정책 클래스입니다.

```cpp
class FUOUUmbrellaVisualPolicy
{
public:
    static EUOUUmbrellaVisualState ResolveVisualState(...);
    static bool ShouldFlipRuntimeVisual(...);
    static FUOUUmbrellaVisualVisibility ResolveVisibility(...);
};
```

월드, 액터, 메시 컴포넌트를 직접 참조하거나 변경하지 않습니다. 입력값을 받으면 결과만 반환하기 때문에 Unreal 월드를 띄우지 않고도 단위 테스트할 수 있습니다.

### 3. 게임플레이 상태를 비주얼 상태로 변환하는 책임 분리

기존에는 `UUOUUmbrellaComponent::ResolveVisualState()`가 다음 매핑을 담당했습니다.

| 게임플레이 상태 | 비주얼 상태 |
|---|---|
| `Closed` | `Closed` |
| `Closed` + 연출 오버라이드 | `ClosedReversed` |
| `Open` | `Open` |
| `UpsideDown` | `OpenReversed` |
| `Pouring` | `OpenReversed` |

이 로직을 다음 정책 함수로 이동했습니다.

```cpp
EUOUUmbrellaVisualState FUOUUmbrellaVisualPolicy::ResolveVisualState(
    EUOUUmbrellaState State,
    bool bUseClosedReversedOverride);
```

`UmbrellaComponent`는 이제 매핑 규칙을 알지 않고 결과만 저장합니다.

```cpp
CurrentVisualState = FUOUUmbrellaVisualPolicy::ResolveVisualState(
    CurrentState,
    bUseClosedReversedVisualOverride);
```

앞으로 `Pouring` 전용 비주얼 상태가 생기거나 새로운 우산 상태가 추가되더라도 상태 매핑 규칙은 정책 클래스에서 관리할 수 있습니다.

### 4. 메시 표시 결과를 하나의 결과 구조체로 통합

새 결과 구조체를 추가했습니다.

```cpp
struct FUOUUmbrellaVisualVisibility
{
    bool bShowClosed = false;
    bool bShowOpen = false;
    bool bShowUpsideDown = false;
    bool bShowRuntime = false;
    bool bFlipRuntime = false;
};
```

각 값의 의미는 다음과 같습니다.

| 값 | 의미 |
|---|---|
| `bShowClosed` | 닫힌 전용 메시를 표시할지 |
| `bShowOpen` | 일반 펼침 메시를 표시할지 |
| `bShowUpsideDown` | 뒤집힌 전용 메시를 표시할지 |
| `bShowRuntime` | 픽업 메시를 복사한 런타임 메시를 표시할지 |
| `bFlipRuntime` | 런타임 메시를 뒤집힌 Transform으로 표시할지 |

표시 규칙은 `ResolveVisibility()`가 한 번에 계산합니다.

```cpp
const FUOUUmbrellaVisualVisibility Visibility =
    FUOUUmbrellaVisualPolicy::ResolveVisibility(
        bHasUmbrella,
        CurrentVisualState,
        bHasDedicatedVisuals,
        UpsideDownVisual != nullptr,
        RuntimeHeldVisual != nullptr,
        bFlipRuntimeHeldVisualWhenUpsideDown);
```

그 결과 `UmbrellaComponent`는 복잡한 우선순위를 다시 판단하지 않고 결과를 적용합니다.

```cpp
ClosedVisual->SetVisibility(Visibility.bShowClosed, true);
OpenVisual->SetVisibility(Visibility.bShowOpen, true);
UpsideDownVisual->SetVisibility(Visibility.bShowUpsideDown, true);
RuntimeHeldVisual->SetVisibility(Visibility.bShowRuntime, true);
```

### 5. 뒤집힌 비주얼 대체 우선순위를 명시

`ResolveVisibility()`에는 뒤집힌 우산을 표시할 때의 우선순위가 모여 있습니다.

```text
1순위: UpsideDownVisual 전용 메시
2순위: RuntimeHeldVisual을 뒤집어서 사용
3순위: OpenVisual을 최종 대체재로 사용
```

#### 전용 뒤집힘 메시가 있는 경우

```text
OpenVisual       = 숨김
UpsideDownVisual = 표시
RuntimeVisual    = 숨김
```

#### 전용 메시가 없고 런타임 메시를 뒤집을 수 있는 경우

```text
OpenVisual       = 숨김
UpsideDownVisual = 없음
RuntimeVisual    = 표시 + 뒤집기
```

#### 전용 메시와 런타임 메시가 모두 없는 경우

```text
OpenVisual       = 표시
```

이 우선순위가 이제 하나의 정책 함수에만 존재하므로, 조건을 수정했는데 한 메시를 숨기는 조건을 빠뜨리는 식의 오류 가능성이 줄었습니다.

### 6. 런타임 메시 회전 판단도 비주얼 상태 기준으로 변경

기존 `ShouldFlipRuntimeHeldVisual()`은 컴포넌트의 게임플레이 상태를 직접 확인했습니다.

```cpp
return bFlipRuntimeHeldVisualWhenUpsideDown
    && (CurrentState == EUOUUmbrellaState::UpsideDown
        || CurrentState == EUOUUmbrellaState::Pouring);
```

변경 후에는 정책이 비주얼 상태만 확인합니다.

```cpp
return bFlipRuntimeVisualWhenReversed
    && VisualState == EUOUUmbrellaVisualState::OpenReversed;
```

비주얼 레이어가 `UpsideDown`과 `Pouring`이라는 게임 규칙을 각각 알 필요가 없어졌습니다. 두 상태가 모두 `OpenReversed`로 해석된다는 결과만 사용합니다.

이는 게임플레이 상태와 표시 상태 사이의 결합도를 낮추는 변경입니다.

### 7. 기존 블루프린트와 에셋 설정을 그대로 유지한 이유

이번 단계에서는 다음 필드를 새 클래스로 옮기지 않았습니다.

- `ClosedVisual`
- `OpenVisual`
- `UpsideDownVisual`
- `SkeletalHeldVisual`
- 각 스켈레탈 소켓 이름
- 상태별 Transform 오프셋
- 상태별 애니메이션 에셋

이 값들은 `UPROPERTY`로 블루프린트와 레벨 인스턴스에 직렬화되어 있을 가능성이 큽니다. 필드의 소유 클래스를 즉시 바꾸면 기존 설정값이 초기화되거나 에셋 마이그레이션이 필요할 수 있습니다.

따라서 이번 순서는 다음과 같습니다.

```text
1. 순수 판단 로직 분리
2. 기존 UPROPERTY 위치 유지
3. 테스트로 판단 결과 고정
4. 이후 Presenter/컴포넌트 분리 시 에셋 마이그레이션 검토
```

### 8. `VisualPolicy` 자동화 테스트 추가

`UOUUmbrellaComponentTests.cpp`에 다음 테스트를 추가했습니다.

```text
UnderOneUmBrella.Player.Umbrella.VisualPolicy
```

검증하는 항목은 다음과 같습니다.

#### 상태 매핑

- `Closed → Closed`
- 닫힘 반전 오버라이드가 있으면 `Closed → ClosedReversed`
- `Pouring → OpenReversed`

#### 전용 뒤집힘 메시

- 일반 펼침 메시가 숨겨지는지
- 뒤집힌 메시가 표시되는지
- 런타임 대체 메시가 표시되지 않는지

#### 런타임 대체 메시

- 뒤집힌 전용 메시가 없을 때 런타임 메시가 표시되는지
- 런타임 메시 뒤집기 결과가 활성화되는지
- 일반 펼침 메시가 동시에 표시되지 않는지

#### 최종 대체 메시

- 전용 메시와 런타임 메시가 모두 없을 때 일반 펼침 메시가 표시되는지

#### 미보유 상태

- 우산을 가지고 있지 않으면 모든 비주얼 결과가 숨김인지

이 테스트는 `UStaticMeshComponent`나 게임 월드를 만들지 않고 정책 함수만 호출합니다. 따라서 실행 비용이 작고 실패 원인을 비주얼 규칙으로 한정할 수 있습니다.

### 9. SOLID 관점에서 달라진 점

#### SRP: 단일 책임 원칙

변경 전 `UmbrellaComponent`는 우산 상태 관리뿐 아니라 비주얼 선택 정책까지 담당했습니다.

변경 후 책임은 다음처럼 나뉩니다.

```text
UUOUUmbrellaComponent
 → 현재 상태와 에셋 참조 보유
 → 정책 결과를 실제 컴포넌트에 적용

FUOUUmbrellaVisualPolicy
 → 상태와 사용 가능한 비주얼을 입력받음
 → 어떤 비주얼을 사용할지 결정
```

#### OCP: 개방-폐쇄 원칙

비주얼 대체 우선순위를 변경할 때 거대한 `UmbrellaComponent`의 상태 처리나 물 붓기 로직을 건드리지 않고 정책 클래스만 수정할 수 있게 됐습니다.

#### DIP: 의존성 역전 원칙을 위한 준비

아직 인터페이스를 주입하는 단계까지 진행하지는 않았지만, 컴포넌트가 구체적인 조건문 대신 정책 결과에 의존하게 됐습니다. 이후 Presenter나 별도 비주얼 컴포넌트로 옮길 수 있는 경계가 만들어졌습니다.

### 10. 검증 결과

다음 검증을 통과했습니다.

- UE 5.7 `UnderOneUmBrellaEditor` 빌드 성공
- `UnderOneUmBrella.Player.Umbrella.StateTransitions` 성공
- `UnderOneUmBrella.Player.Umbrella.PourVisualState` 성공
- `UnderOneUmBrella.Player.Umbrella.VisualPolicy` 성공
- `git diff --check` 통과

### 11. 아직 남아 있는 비주얼 책임

이번 작업으로 비주얼의 **선택 정책**은 분리했지만 실제 표시 작업은 아직 `UmbrellaComponent`에 남아 있습니다.

- 메시 컴포넌트 `SetVisibility()` 호출
- 스켈레탈 메시 부착 대상과 소켓 선택
- 스켈레탈 Transform 적용
- 애니메이션 에셋 선택 및 재생
- 런타임 픽업 메시 생성과 머티리얼 복사
- 런타임 메시 Transform 적용

다음 리팩토링에서는 `RefreshSkeletalVisual()`의 소켓 선택, 부착, Transform, 애니메이션 적용 책임을 별도 Presenter 또는 비주얼 헬퍼로 분리하는 것이 안전합니다.

---

## 3차 리팩토링: 스켈레탈 비주얼 Presenter 분리

### 작업 목적

2차 작업에서 “어떤 비주얼을 보여줄 것인가”라는 선택 정책을 분리했지만, `UUOUUmbrellaComponent::RefreshSkeletalVisual()`에는 스켈레탈 메시를 실제로 제어하는 코드가 그대로 남아 있었습니다.

기존 함수는 다음 작업을 한꺼번에 담당했습니다.

- 스켈레탈 메시 충돌과 오버랩 설정
- 우산 미보유 시 메시 숨김
- 캐릭터 메시와 우산 소켓 결정
- 캐릭터 메시, 앵커, 픽업 부착점, 루트 컴포넌트 순서로 부착 대상 탐색
- 상태별 상대 Transform 적용
- `UUOUUmbrellaAnimInstance`에 우산 상태 전달
- 직접 애니메이션 재생 여부 판단
- 같은 애니메이션의 중복 재생 방지

이 책임을 `FUOUUmbrellaSkeletalVisualPresenter`로 이동했습니다.

### 추가된 파일

- `Source/UnderOneUmBrella/Player/UOUUmbrellaSkeletalVisualPresenter.h`
- `Source/UnderOneUmBrella/Player/UOUUmbrellaSkeletalVisualPresenter.cpp`

Presenter는 Unreal 컴포넌트를 새로 소유하지 않는 일반 C++ 클래스입니다. `UmbrellaComponent`가 전달한 요청을 받아 이미 존재하는 `USkeletalMeshComponent`에 표시 상태를 적용합니다.

### 상태별 Variant 구조 추가

스켈레탈 우산은 비주얼 상태마다 서로 다른 소켓, Transform, 애니메이션을 사용합니다.

```cpp
struct FUOUUmbrellaSkeletalVisualVariant
{
    FName SocketName;
    FTransform RelativeTransform;
    UAnimationAsset* Animation;
};
```

네 가지 비주얼 상태의 설정은 다음 컨테이너로 묶었습니다.

```cpp
struct FUOUUmbrellaSkeletalVisualVariants
{
    FUOUUmbrellaSkeletalVisualVariant Closed;
    FUOUUmbrellaSkeletalVisualVariant Open;
    FUOUUmbrellaSkeletalVisualVariant ClosedReversed;
    FUOUUmbrellaSkeletalVisualVariant OpenReversed;

    const FUOUUmbrellaSkeletalVisualVariant& Resolve(
        EUOUUmbrellaVisualState VisualState) const;
};
```

기존에는 `UmbrellaComponent`에 아래 세 개의 switch 함수가 각각 존재했습니다.

```text
GetSkeletalVisualSocketName()
GetSkeletalVisualOffset()
GetSkeletalVisualAnimation()
```

세 함수가 모두 같은 `VisualState`를 기준으로 별도의 switch를 수행했기 때문에 새로운 상태를 추가할 때 한 함수를 빠뜨릴 수 있었습니다. 이제 `Variants.Resolve()` 한 번으로 소켓, Transform, 애니메이션이 같은 Variant에서 함께 선택됩니다.

### Presenter 요청 객체

Presenter에 필요한 입력을 다음 요청 구조체로 명시했습니다.

```cpp
struct FUOUUmbrellaSkeletalVisualRequest
{
    USkeletalMeshComponent* Visual;
    USceneComponent* HeldVisualAnchor;
    USceneComponent* PickupAttachPoint;
    AActor* Owner;

    bool bHasUmbrella;
    bool bAttachToOwnerMeshSocket;
    bool bPlayAnimationDirectly;

    EUOUUmbrellaState State;
    EUOUUmbrellaDirectionState DirectionState;
    EUOUUmbrellaVisualState VisualState;
    FUOUUmbrellaSkeletalVisualVariant Variant;
};
```

이 구조를 통해 Presenter가 `UUOUUmbrellaComponent` 전체를 참조하지 않아도 필요한 작업을 수행할 수 있습니다. 컴포넌트의 수많은 public 필드에 직접 결합되지 않고 명시적인 입력 데이터에만 의존합니다.

### 부착 책임 이동

Presenter는 다음 우선순위로 부착 대상을 결정합니다.

```text
1. 캐릭터 SkeletalMesh의 상태별 소켓
2. HeldVisualAnchor
3. PickupAttachPoint
4. Owner의 RootComponent
```

캐릭터 메시를 사용할 수 있으면 Variant에 들어 있는 상태별 소켓 이름을 사용합니다.

```cpp
if (ACharacter* OwnerCharacter = Cast<ACharacter>(Request.Owner))
{
    AttachParent = OwnerCharacter->GetMesh();
    AttachSocketName = Request.Variant.SocketName;
}
```

그렇지 않으면 기존 앵커 대체 순서를 유지합니다. 동작을 바꾸지 않고 코드의 소유 위치만 옮긴 것입니다.

### AnimInstance 상태 전달 이동

기존에는 `UmbrellaComponent`가 `UUOUUmbrellaAnimInstance`를 직접 찾아 상태를 전달했습니다.

```cpp
UmbrellaAnimInstance->SetUmbrellaState(
    bHasUmbrella,
    CurrentState,
    CurrentDirectionState,
    CurrentVisualState);
```

이제 Presenter가 요청에 담긴 상태를 AnimInstance에 전달합니다. 그 결과 `UmbrellaComponent.cpp`는 `UOUUmbrellaAnimInstance.h`에 직접 의존하지 않게 됐습니다.

### 직접 애니메이션 재생 상태 분리

같은 애니메이션을 `RefreshVisuals()`가 호출될 때마다 다시 시작하지 않도록 기존에는 다음 두 필드를 컴포넌트가 직접 조작했습니다.

```cpp
bHasAppliedSkeletalVisualAnimation
LastAppliedSkeletalVisualAnimation
```

Presenter에는 이를 전달하기 위한 실행 상태 구조체를 추가했습니다.

```cpp
struct FUOUUmbrellaSkeletalVisualPlaybackState
{
    bool bHasAppliedAnimation;
    UAnimationAsset* LastAppliedAnimation;
};
```

Presenter는 다음 경우에만 애니메이션을 재생합니다.

```text
직접 재생 옵션이 활성화되어 있음
AND 현재 Variant에 애니메이션이 있음
AND 아직 재생하지 않았거나 이전 애니메이션과 다름
```

우산을 제거하거나 직접 재생 옵션을 끄면 PlaybackState를 초기화합니다.

### 변경 후 `RefreshSkeletalVisual()`의 역할

변경 전에는 실제 표시 과정을 모두 수행했지만, 변경 후에는 다음 세 단계만 담당합니다.

```text
1. 기존 UPROPERTY를 Variant 집합으로 조립
2. 현재 상태와 참조를 Request에 담음
3. Presenter에 적용을 위임하고 PlaybackState 결과를 저장
```

핵심 호출은 다음과 같습니다.

```cpp
FUOUUmbrellaSkeletalVisualPresenter::Apply(Request, PlaybackState);
```

`UmbrellaComponent`는 여전히 블루프린트에 직렬화된 에셋 설정을 보유하지만, 스켈레탈 메시를 어떻게 부착하고 재생할지는 알 필요가 줄었습니다.

### 자동화 테스트 추가

다음 테스트를 추가했습니다.

```text
UnderOneUmBrella.Player.Umbrella.SkeletalVisualPresenter
```

검증 항목은 다음과 같습니다.

- `Closed`가 닫힘 Variant를 선택하는지
- `Open`이 펼침 Variant를 선택하는지
- `ClosedReversed`가 닫힘 반전 Variant를 선택하는지
- `OpenReversed`가 펼침 반전 Variant를 선택하는지
- 우산 미보유 시 스켈레탈 메시가 숨겨지는지
- 충돌이 `NoCollision`으로 설정되는지
- 오버랩 이벤트가 비활성화되는지
- 우산 제거 시 직접 애니메이션 재생 상태가 초기화되는지

### SOLID 관점

#### SRP

`UUOUUmbrellaComponent`는 상태와 설정을 조립하고, Presenter는 실제 스켈레탈 비주얼 표시를 담당합니다.

#### OCP

부착 방식이나 애니메이션 적용 정책을 변경할 때 물 붓기, 빗물 차단, 오디오 상태 전이 코드를 수정하지 않아도 되는 경계가 생겼습니다.

#### ISP

Presenter는 거대한 `UUOUUmbrellaComponent` 전체가 아니라 작업에 필요한 작은 `Request` 구조체에만 의존합니다.

### 검증 결과

- UE 5.7 `UnderOneUmBrellaEditor` 빌드 성공
- `StateTransitions` 성공
- `PourVisualState` 성공
- `VisualPolicy` 성공
- `SkeletalVisualPresenter` 성공
- `git diff --check` 통과

### 이후 남은 주요 분리 대상

우선순위는 다음과 같습니다.

1. 런타임 픽업 메시 생성, 메시/머티리얼 복사, Transform 적용
2. 물 붓기 세션과 물방울 누적·생성·충돌 전달
3. 빗물 차단 판정과 RainReceiver 연동
4. 우산 오디오 큐와 비 차단 루프 상태
5. 디버그 데이터 수집과 월드 드로잉
6. 최종적으로 우산 상태 전이 자체를 명시적인 상태 머신으로 분리

다음 작업은 런타임 픽업 메시 책임을 같은 비주얼 계층으로 옮기는 것이 가장 자연스럽습니다. 이 작업까지 끝내면 `UmbrellaComponent`에서 비주얼 관련 구현 대부분이 빠지고, 이후 게임플레이 기능 분리를 시작할 수 있습니다.

---

## 4차 리팩토링: 런타임 픽업 비주얼 Presenter 분리

### 작업 목적

월드에 놓인 우산을 획득하면 픽업 액터의 `UStaticMeshComponent`에서 메시, 머티리얼, 상대 스케일을 읽어 플레이어 손의 런타임 메시로 복사합니다. 기존에는 이 전체 과정이 `UUOUUmbrellaComponent` 안에 들어 있었습니다.

기존 컴포넌트가 담당하던 작업은 다음과 같습니다.

- `RuntimeHeldVisual` 동적 생성과 등록
- 충돌, 오버랩, 그림자, 초기 가시성 설정
- `PickupAttachPoint` 또는 루트 컴포넌트에 부착
- 픽업 메시와 모든 머티리얼 수집
- 기본 메시 fallback 적용
- 앵커 위치·회전과 픽업 스케일을 조합한 기본 Transform 계산
- 뒤집힘/붓기 상태에서 회전과 위치 오프셋 적용
- 최종 메시와 머티리얼을 런타임 컴포넌트에 적용

이 책임을 `FUOUUmbrellaRuntimeVisualPresenter`로 분리했습니다.

### 추가된 파일

- `Source/UnderOneUmBrella/Player/UOUUmbrellaRuntimeVisualPresenter.h`
- `Source/UnderOneUmBrella/Player/UOUUmbrellaRuntimeVisualPresenter.cpp`

### 픽업 비주얼 에셋 묶음

픽업에서 읽어야 하는 데이터를 하나의 구조체로 묶었습니다.

```cpp
struct FUOUUmbrellaRuntimeVisualAssets
{
    UStaticMesh* Mesh;
    TArray<UMaterialInterface*> Materials;
    FVector SourceRelativeScale;
};
```

이전에는 `UmbrellaComponent`가 직접 머티리얼 슬롯을 순회했습니다.

```cpp
for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
{
    Materials.Add(SourceMeshComponent->GetMaterial(MaterialIndex));
}
```

이제 Presenter의 `CaptureAssets()`가 이 작업을 담당합니다.

```cpp
const FUOUUmbrellaRuntimeVisualAssets Assets =
    FUOUUmbrellaRuntimeVisualPresenter::CaptureAssets(SourceMeshComponent);
```

`UmbrellaComponent`는 픽업 메시 내부의 머티리얼 슬롯 구조를 알 필요 없이 캡처된 데이터만 전달합니다.

### 런타임 메시 컴포넌트 생성 분리

`EnsureVisual()`은 다음 생명주기 작업을 담당합니다.

```text
기존 RuntimeHeldVisual이 있으면 그대로 반환
 → Owner가 없으면 생성하지 않음
 → UStaticMeshComponent 생성
 → Owner의 InstanceComponent로 등록
 → 충돌과 오버랩 비활성화
 → 초기 가시성 숨김
 → PickupAttachPoint 또는 RootComponent에 부착
 → 컴포넌트 등록
 → 초기 상대 Transform 적용
```

호출부는 다음처럼 단순해졌습니다.

```cpp
RuntimeHeldVisual = FUOUUmbrellaRuntimeVisualPresenter::EnsureVisual(
    GetOwner(),
    PickupAttachPoint,
    RuntimeHeldVisual,
    RuntimeHeldVisualBaseRelativeTransform);
```

컴포넌트의 존재 여부를 관리하는 진입점 `EnsureRuntimeHeldVisual()`은 유지했습니다. 하지만 실제 생성 방법과 표시 전용 설정은 Presenter가 담당합니다.

### 기본 Transform 계산 분리

기본 Transform은 다음 입력을 조합합니다.

- `HeldVisualAnchor`의 상대 위치
- `HeldVisualAnchor`의 상대 회전
- `HeldVisualRelativeScale`
- 픽업 메시의 상대 스케일
- `bUsePickupMeshRelativeScale` 옵션

계산은 `CalculateBaseRelativeTransform()`으로 이동했습니다.

```cpp
FTransform CalculateBaseRelativeTransform(
    const FTransform& AnchorRelativeTransform,
    const FVector& HeldVisualRelativeScale,
    const FVector& SourceRelativeScale,
    bool bUseSourceRelativeScale);
```

픽업 스케일을 사용하면 축별로 다음 계산을 수행합니다.

```text
최종 Scale.X = HeldScale.X × PickupScale.X
최종 Scale.Y = HeldScale.Y × PickupScale.Y
최종 Scale.Z = HeldScale.Z × PickupScale.Z
```

옵션이 꺼져 있으면 픽업 스케일을 무시하고 `HeldVisualRelativeScale`만 사용합니다.

이 함수는 메시 컴포넌트를 변경하지 않는 순수 계산 함수라 월드 없이 테스트할 수 있습니다.

### 상태별 뒤집힘 Transform 분리

기존 `ApplyRuntimeHeldVisualStateTransform()`은 `UmbrellaComponent` 내부에서 회전과 위치 오프셋을 직접 계산했습니다.

이 함수는 제거하고 다음 두 함수로 역할을 나눴습니다.

```cpp
CalculateStateRelativeTransform(...);
ApplyStateTransform(...);
```

`CalculateStateRelativeTransform()`은 기본 Transform에서 최종 Transform을 계산합니다. `OpenReversed` 상태이고 뒤집기 옵션이 켜져 있을 때만 다음 보정을 적용합니다.

```text
최종 회전 = 기본 회전 × UpsideDown 회전 오프셋
최종 위치 = 기본 위치 + UpsideDown 위치 오프셋
```

`ApplyStateTransform()`은 계산된 결과를 실제 `UStaticMeshComponent`에 적용합니다.

비주얼 상태 판정은 앞에서 분리한 `FUOUUmbrellaVisualPolicy::ShouldFlipRuntimeVisual()`을 재사용합니다. 따라서 런타임 Presenter가 `UpsideDown`과 `Pouring`이라는 게임플레이 상태를 직접 알 필요가 없습니다.

### 메시와 머티리얼 적용 분리

`ApplyAssets()`는 캡처한 데이터를 런타임 비주얼에 적용합니다.

```cpp
Visual->SetStaticMesh(
    Assets.Mesh != nullptr ? Assets.Mesh : DefaultMesh);
```

픽업 메시가 없으면 기존 `DefaultHeldMesh`를 사용합니다. 픽업에서 읽은 머티리얼은 동일한 슬롯 인덱스에 적용합니다.

```cpp
for (int32 MaterialIndex = 0;
     MaterialIndex < Assets.Materials.Num();
     ++MaterialIndex)
{
    Visual->SetMaterial(MaterialIndex, Assets.Materials[MaterialIndex]);
}
```

### 변경 후 획득 흐름

```text
AcquireUmbrellaFromMeshComponent
 → 우산 소유 상태 변경
 → RuntimeVisualPresenter.CaptureAssets
 → EnsureRuntimeHeldVisual
   → RuntimeVisualPresenter.EnsureVisual
 → RuntimeVisualPresenter.ApplyAssets
 → RuntimeVisualPresenter.CalculateBaseRelativeTransform
 → RuntimeVisualPresenter.ApplyStateTransform
 → RefreshVisuals
```

`UmbrellaComponent`는 이제 획득 흐름을 조율하지만 픽업 메시의 복사 방법과 Transform 계산 공식은 직접 구현하지 않습니다.

### 자동화 테스트 추가

다음 테스트를 추가했습니다.

```text
UnderOneUmBrella.Player.Umbrella.RuntimeVisualPresenter
```

검증 항목은 다음과 같습니다.

- 앵커 위치가 기본 Transform에 유지되는지
- 앵커 회전이 기본 Transform에 유지되는지
- 손 비주얼 스케일과 픽업 스케일이 축별로 곱해지는지
- 픽업 스케일 사용 옵션을 끄면 픽업 스케일이 무시되는지
- `OpenReversed`에서 위치 오프셋과 추가 회전이 적용되는지
- 일반 `Open` 상태에서는 기본 Transform이 유지되는지
- 픽업의 `UStaticMesh`와 상대 스케일이 캡처되는지
- 캡처한 메시가 대상 런타임 비주얼에 적용되는지
- 기존 런타임 비주얼이 있으면 Owner 없이도 그대로 재사용되는지

### SOLID 관점

#### SRP

런타임 픽업 비주얼의 생성, 에셋 복사, Transform 계산이 우산 게임플레이 컴포넌트에서 분리됐습니다.

#### OCP

앞으로 픽업 외형 복사 규칙이나 뒤집힘 보정을 변경해도 물 붓기, 빗물 차단, 오디오 상태 코드를 수정할 필요가 줄었습니다.

#### ISP

Presenter의 각 함수는 필요한 입력만 받습니다. 전체 `UUOUUmbrellaComponent`를 전달하지 않으므로 거대한 인터페이스에 의존하지 않습니다.

### 검증 결과

- UE 5.7 `UnderOneUmBrellaEditor` 빌드 성공
- `StateTransitions` 성공
- `PourVisualState` 성공
- `VisualPolicy` 성공
- `SkeletalVisualPresenter` 성공
- `RuntimeVisualPresenter` 성공
- `git diff --check` 통과

### 비주얼 분리 이후 남은 주요 책임

비주얼 선택 정책, 스켈레탈 표시, 런타임 픽업 표시가 분리됐습니다. 이제 `UmbrellaComponent`에서 가장 큰 책임은 게임플레이 영역입니다.

다음 권장 순서는 다음과 같습니다.

1. 물 붓기 세션과 물방울 누적·생성·충돌 전달
2. 빗물 차단 판정과 `RainReceiver` 연동
3. 오디오 큐와 비 차단 루프 상태
4. 디버그 상태 기록과 월드 드로잉
5. 최종 상태 머신 분리

다음 단계에서는 물 붓기 로직을 한 번에 모두 옮기기 전에, 먼저 물방울 누적량과 생성 주기를 관리하는 작은 세션 객체를 분리하는 것이 안전합니다.
