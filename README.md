# Under One Umbrella | 한 우산 아래

> 하나의 우산으로 비를 막고, 물을 모으고, 빛을 반사하며 길을 찾아가는 3D 퍼즐 어드벤처.

**한 우산 아래**는 우산과 주변 환경의 상호작용을 활용해 스테이지를 해결하는 Unreal Engine 팀 프로젝트입니다.

우산을 펼치거나 뒤집고, 모은 물을 붓고, 빛의 방향을 바꾸며 퍼즐의 해답을 찾아갑니다. 주변 장치와 공간을 관찰하고, 같은 도구를 다른 방식으로 사용하는 것이 플레이의 중심입니다.

## 주요 플레이

### 우산을 활용한 상호작용

- **비 막기** — 우산을 펼쳐 비를 막습니다.
- **물 모으기와 붓기** — 우산에 물을 모아 다른 장치에 전달합니다.
- **빛 반사와 색상 퍼즐** — 빛을 원하는 방향으로 유도하고 대상의 색을 바꿉니다.
- **바람 활용** — 우산과 바람의 상호작용을 이용합니다.
- **형태와 방향 전환** — 상황에 맞게 우산의 상태를 바꿉니다.

### 환경 퍼즐과 탐험

발판, 버튼, 물레방아, 이동 장치 등 다양한 오브젝트를 연결해 길을 엽니다. 스테이지 선택 공간에서 다음 장소로 이동하고, NPC 대화와 말풍선 안내를 통해 플레이에 필요한 단서를 얻습니다.

## 개발 환경

| 구분 | 사용 기술 |
| --- | --- |
| 게임 엔진 | Unreal Engine 5.7 |
| 개발 언어 | C++, Blueprint |
| 개발 및 빌드 환경 | Windows, Visual Studio 2022 |
| UI | UMG / Slate |
| 입력 | Enhanced Input |
| 시각 효과 | Niagara |
| 버전 관리 | Git / GitHub / Git LFS |

## 주요 구현 시스템

- **우산 상태 관리**  
  펼침·접힘·뒤집기·물 붓기·빛 반사 상태와 관련 상호작용을 관리합니다.

- **환경 상호작용**  
  비, 물, 바람, 빛을 주고받는 액터와 컴포넌트로 퍼즐을 구성합니다.

- **퍼즐 조건 및 결과 연결**  
  `ConditionGroup`을 통해 여러 조건을 묶고, 조건 충족 여부에 따라 장치 활성화와 후속 동작을 연결합니다.

- **스테이지 선택과 진행 정보**  
  데이터 기반 스테이지 정보, 미리보기, 보상 상태와 레벨 이동을 관리합니다.

- **NPC 대화 및 연출**  
  근접 대화, 말풍선, NPC 행동과 카메라 연출을 구성합니다.

- **개발 지원 기능**  
  퍼즐 디버깅 도구와 자동화 테스트를 포함합니다.

## 프로젝트 실행

### 1. 준비

- Unreal Engine **5.7.4**
- Visual Studio **2022**
- Visual Studio의 **C++를 사용한 게임 개발** 워크로드와 Windows SDK
- Git 및 **Git LFS**

추가 개발 구성 요소는 저장소의 `.vsconfig`를 참고하세요.

### 2. 저장소 받기

```bash
git lfs install
git clone https://github.com/flint21c1996/UnderOneUmbrella.git
cd UnderOneUmbrella
git lfs pull
```

이 프로젝트는 `.uasset`, `.umap`, `.fbx` 파일을 Git LFS로 관리합니다. 에셋까지 정상적으로 내려받은 뒤 프로젝트를 열어주세요.

### 3. 빌드 및 에디터 실행

1. `UnderOneUmBrella.uproject`를 우클릭합니다.
2. **Generate Visual Studio project files**를 실행합니다. Windows 11에서는 **추가 옵션 표시** 안에 있을 수 있습니다.
3. 생성된 `UnderOneUmBrella.sln`을 Visual Studio로 엽니다.
4. 빌드 구성을 **Development Editor / Win64**로 선택하고 빌드합니다.
5. `UnderOneUmBrella.uproject`를 열어 에디터를 실행합니다.

기본 시작 맵은 `Content/UOU/Maps/TitleMap`입니다.

## 주요 폴더

```text
UnderOneUmbrella/
├─ Config/                         # 프로젝트 설정
├─ Content/
│  ├─ ArtSource/                   # 아트 리소스
│  └─ UOU/
│     ├─ Maps/                     # 게임 맵
│     ├─ DataTable/                # 스테이지·대화 등 데이터
│     └─ UI/                       # UI 리소스
├─ Source/
│  ├─ UnderOneUmBrella/
│  │  ├─ Player/                   # 플레이어 및 우산
│  │  ├─ Interaction/              # 상호작용
│  │  ├─ Puzzle/                   # 퍼즐 조건 및 결과
│  │  ├─ World/                    # 환경 장치와 월드 액터
│  │  ├─ Game/                     # 게임 진행 및 스테이지 관리
│  │  ├─ UI/                       # UI 로직
│  │  ├─ Audio/                    # 오디오
│  │  ├─ Debug/                    # 개발 지원 기능
│  │  └─ Tests/                    # 자동화 테스트
│  └─ UnderOneUmBrellaEditor/      # 에디터 전용 모듈
└─ UnderOneUmBrella.uproject
```

## 협업

기능별 브랜치에서 작업하고 Pull Request를 통해 변경사항을 통합합니다.

Unreal 에셋은 일반 텍스트 코드처럼 병합하기 어려우므로, 같은 맵이나 블루프린트를 함께 수정할 때는 작업 범위를 먼저 공유합니다. 커밋 전에는 에디터에서 저장한 뒤 변경 파일을 확인합니다.

---

현재 개발 중인 프로젝트이며, 플레이 구성과 콘텐츠는 변경될 수 있습니다.
