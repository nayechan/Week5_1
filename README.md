# ✦ WEEK 05: 엔진 최적화 기법 분석

## 1. 개요

이번 주차에서는 다양한 최적화 기법을 구현하고 이해하는 것을 목표로 합니다. 주요 키워드는 **Frustum Culling**, **Occlusion Culling**, **Scene Graph**, **SIMD**, 공간 분할(**BVH**, **Octree**), **LOD**, **RAII**, **Performance Profiling**, 렌더링 최적화(**Sorting**, **Draw Call**, **Render State**)입니다.

특히 5만 개의 `StaticMeshComponent`에 대한 **Picking 성능 측정** 시나리오를 통해, 대규모 씬에서 발생하는 병목 현상을 이해하고 이를 어떻게 효율적으로 해결하는지에 대해 중점적으로 다룹니다.

---

## 2. 핵심 최적화 기법 분석

### 2.1. Frustum Culling

카메라의 시야각 밖에 있는 물체를 렌더링 파이프라인에서 조기에 제외하는 기법입니다.

- **구현**:
    - `Source/Render/Spatial/Public/Frustum.h`: `FPlane` 6개로 구성된 절두체(`FFrustum`)를 정의합니다. `ConstructFromViewProjectionMatrix` 함수를 통해 카메라의 View-Projection 행렬로부터 절두체를 생성합니다. AABB, 구(Sphere) 등이 절두체 내부에 있는지 판별하는 `IsBoxInFrustum`, `IsSphereInFrustum` 등의 함수를 제공합니다.
    - `Source/Render/Spatial/Public/Octree.h`: Octree는 Frustum Culling을 가속화하는 데 핵심적인 역할을 합니다. `QueryFrustum` 메서드는 절두체와 겹치는 노드만을 순회하여 렌더링 후보 객체 목록을 매우 빠르게 필터링합니다. 이를 통해 씬의 모든 객체를 개별적으로 검사할 필요가 없어집니다.
    - `Source/Render/Renderer/Public/Renderer.h`: `RenderLevel` 함수에서 현재 카메라 정보를 바탕으로 Frustum을 생성하고, 이를 Octree에 전달하여 보이는 객체 목록을 얻어와 렌더링을 수행합니다. 또한, HZB를 이용한 Occlusion Culling을 구현하여 Frustum 내에 있지만 다른 객체에 의해 가려진 객체까지 추가로 제외시킵니다. `IsOccluded` 함수가 이 역할을 담당합니다.

### 2.2. Occlusion Culling

카메라 시야각 내에 있지만 다른 객체에 완전히 가려져 보이지 않는 객체를 렌더링하지 않는 최적화 기법입니다.

- **구현**:
    - `HZB(Hierarchical-Z Buffer)`: 계층적 Z-버퍼를 사용하여, 렌더링된 씬의 깊이 버퍼(Depth Buffer)를 복사하고, Compute Shader(HZBComputeShader.hlsl)를 통해 깊이 값의 피라미드(Mipmap)를 생성합니다.
    - 상위 레벨의 텍셀은 하위 레벨 텍셀들 중 가장 큰 깊이 값(카메라에서 가장 먼 값)을 저장합니다.
    - 이 과정은 `Renderer.h`의 `IsOccluded` 함수와 관련 HZB 생성 함수들을 통해 관리되며, 특히 복잡한 씬에서 렌더링 성능을 크게 향상시킵니다.

### 2.3. Scene Graph

씬을 구성하는 객체들을 부모-자식 관계의 계층 구조(Hierarchy)로 관리하는 자료 구조입니다.

- **구현**:
    - `Source/Component/Public/SceneComponent.h`: 엔진의 씬 그래프의 핵심 요소입니다.
        - `ParentAttachment` (부모)와 `Children` (자식 배열) 멤버를 통해 계층 구조를 형성합니다.
        - `RelativeLocation`, `RelativeRotation`, `RelativeScale3D` 등 부모에 대한 상대적인 변환 값을 저장합니다.
        - **Dirty Flag 최적화**: `bIsTransformDirty` 플래그를 사용하여, 컴포넌트의 Transform이 변경되었을 때만 월드 변환 행렬을 다시 계산합니다 (`GetWorldTransformMatrix`). 부모의 Transform이 변경되면 모든 자식들을 Dirty로 표시하여 계층 구조 전체에 변경사항을 효율적으로 전파합니다. 이는 매 프레임마다 불필요한 행렬 계산을 방지하는 중요한 최적화입니다.

### 2.4. SIMD (Single Instruction, Multiple Data)

하나의 명령어로 여러 개의 데이터를 동시에 처리하는 기술로, 주로 벡터 및 행렬 연산과 같은 수학 계산을 가속화하는 데 사용됩니다.

- **구현**:
    - `Source/Global/Vector.h`: `FVector` 클래스의 핵심 수학 함수들이 SSE/AVX 내장 함수(Intrinsic)를 사용하여 구현되었습니다.
        - **`Length()`**, **`LengthSquared()`**, **`Dot()`**, **`Normalize()`** 등의 함수 내부에서 `__m128` 데이터 타입과 `_mm_mul_ps`, `_mm_hadd_ps`, `_mm_rsqrt_ss` 같은 SIMD 명령어를 직접 사용하여 CPU 연산을 최적화하고 있습니다.

### 2.5. 공간 분할 (Spatial Partitioning)

넓은 공간을 작은 영역으로 나누어 관리함으로써, 특정 위치나 영역에 대한 검색, 충돌 감지, 컬링 등의 연산을 가속화하는 기법입니다. 우리 엔진에서는 Octree와 BVH를 핵심적으로 사용합니다.

#### 2.5.1. Octree

공간을 정육면체 형태로 8개의 자식 노드로 재귀적으로 분할하는 구조입니다. 주로 동적인 객체들을 관리하고, 넓은 영역에 대한 쿼리(Frustum Culling 등)에 유리합니다.

- **구현**:
    - `Source/Render/Spatial/Public/Octree.h`: `FOctree` 클래스는 씬에 존재하는 `UPrimitiveComponent`들을 관리합니다.
        - `Insert`, `Remove`를 통해 동적으로 객체를 추가/제거할 수 있습니다.
        - `QueryFrustum` 메서드를 통해 Frustum Culling을 효율적으로 수행합니다.
        - **계층적 노드 컬링**: `ConsecutiveOccludedFrames` 변수를 통해 특정 노드가 N 프레임 연속으로 화면에 보이지 않을 경우, 해당 노드 전체를 컬링 대상에서 제외하는 고급 최적화 기법이 포함되어 있습니다.

#### 2.5.2. BVH (Bounding Volume Hierarchy)

객체들을 감싸는 경계 볼륨(Bounding Volume)들을 계층적인 트리 구조로 구성하는 방식입니다. 주로 정적인 객체들에 대해 사용되며, 특히 레이캐스팅(Ray-Casting) 연산에 매우 효율적입니다.

- **구현**:
    - **`FStaticMeshBVH` (Bottom-Level Acceleration Structure - BLAS)**
        - `Source/Utility/Public/StaticMeshBVH.h`: 단일 `StaticMesh`를 구성하는 수많은 삼각형(Triangle)들에 대한 BVH를 구축합니다.
        - Picking 시, 레이가 메쉬와 충돌했는지 판별할 때 모든 삼각형을 검사하는 대신, BVH를 따라 내려가며 소수의 삼각형 후보군만을 빠르게 추려냅니다.
        - **SoA (Structure-of-Arrays)**: 삼각형의 정점, 엣지 등의 데이터를 `TriV0X`, `TriE1X` 와 같이 배열로 분리하여 저장함으로써, 순회 시 캐시 효율성을 극대화합니다.
    - **`FSceneBVH` (Top-Level Acceleration Structure - TLAS)**
        - `Source/Utility/Public/SceneBVH.h`: 씬에 존재하는 여러 `UPrimitiveComponent` 객체들에 대한 상위 레벨 BVH를 구축합니다.
        - Picking 시, 먼저 `SceneBVH`를 통해 레이와 충돌 가능성이 있는 소수의 객체 후보군을 선별하고, 그 후에 해당 객체의 `StaticMeshBVH`를 검사하는 2단계 계층 구조를 통해 엄청난 가속을 얻습니다.
        - **SAH (Surface Area Heuristic)**: BVH 빌드 시 표면적 휴리스틱을 사용하여คุณภาพ 높은 트리를 생성합니다.
        - **Refit**: 객체가 이동했을 때 전체 BVH를 재빌드하지 않고, 기존 구조를 유지한 채 AABB만 업데이트하는 `Refit` 기능을 지원하여 동적인 씬에 더 빠르게 대응할 수 있습니다.

#### 2.5.3. K-d tree

K-d 트리는 공간을 축 기준으로 분할하는 또 다른 공간 분할 자료구조이지만, 현재 프로젝트에는 도입하지 않았습니다. 대신 BVH와 Octree를 중심으로 공간 분할을 구현하였습니다.

### 2.6. LOD (Level of Detail)

카메라와의 거리에 따라 객체의 모델을 다른 상세도로 교체하여 렌더링 부하를 조절하는 최적화 기법입니다. 가까이 있는 객체는 폴리곤 수가 많은 고품질 모델(High-Poly), 멀리 있는 객체는 폴리곤 수가 적은 저품질 모델(Low-Poly)로 렌더링하여 효율을 높입니다.

- **구현**:
    - `QEM 알고리즘`: 먼저 원본 메쉬의 모든 정점(Vertex)에 대해 오차 행렬(Quadric Matrix)을 계산합니다.
    - 메시의 모든 간선(Edge)에 대해, 해당 간선을 축소했을 때 발생할 오차, 즉 비용(Cost)을 계산합니다.
    - 모든 간선의 축소 비용을 계산한 후, 이들을 '최소 힙(Min-Heap)'과 같은 우선순위 큐에 넣어 비용이 가장 낮은 순서대로 정렬합니다.
    - 하나의 간선이 축소되면 주변 메쉬 구조가 변경되므로, 새로 만들어진 정점에 연결된 간선들의 축소 비용을 다시 계산하여 우선순위 큐를 업데이트합니다.

### 2.7. RAII (Resource Acquisition Is Initialization)

"자원 획득은 초기화다"라는 C++의 핵심 원칙 중 하나입니다. 객체가 생성될 때(초기화) 자원을 획득하고, 객체가 소멸될 때(스코프를 벗어날 때) 자동으로 자원을 해제하도록 하여 자원 누수를 방지하고 코드를 안전하게 만듭니다.

- **구현**:
    - `Source/Utility/Public/ScopeCycleCounter.h`: `FScopeCycleCounter` 클래스는 RAII의 완벽한 예시입니다.
        - 생성자에서 `FPlatformTime::Cycles64()`를 호출하여 시간 측정을 시작합니다.
        - 소멸자(컴파일러에 의해 스코프 끝에 자동으로 호출됨)에서 현재 시간과의 차이를 계산하여 성능 통계를 기록하고 종료합니다.
        - 이를 통해 개발자는 수동으로 타이머를 멈추는 코드를 작성할 필요가 없으며, 함수가 중간에 예외를 던지거나 반환되어도 항상 안전하게 성능 측정이 종료됨을 보장받습니다.

### 2.8. Performance Profiling

코드의 특정 구간이 실행되는 데 걸리는 시간을 측정하여 성능 병목 지점을 찾아내는 과정입니다.

- **구현**:
    - `Source/Utility/Public/ScopeCycleCounter.h`: 위에서 설명한 `FScopeCycleCounter`가 프로파일링의 핵심 도구입니다.
    - `GetPickingStatId()`, `GetSceneBVHTraverseStatId()` 등과 같이 측정할 기능별로 `TStatId`를 부여하여 통계를 구분합니다.
    - `#ifdef ENABLE_BVH_STATS` 와 같은 전처리기 매크로를 사용하여, 릴리즈 빌드에서는 프로파일링 관련 코드가 컴파일되지 않도록 하여 오버헤드를 제거할 수 있습니다.
    - `Source/Render/UI/Overlay/Public/StatOverlay.h` (파일 목록에서 확인)는 이렇게 수집된 통계 정보를 화면에 시각적으로 표시하는 역할을 합니다.

### 2.9. 렌더링 최적화

#### 2.9.1. Sorting

렌더링할 객체들을 특정 기준(셰이더, 텍스처, 거리 등)에 따라 정렬함으로써, GPU의 상태 변경(Render State Change)을 최소화하여 Draw Call의 효율을 높이는 기법입니다. 예를 들어, 같은 셰이더와 텍스처를 사용하는 물체들을 모아서 한 번에 그리면 GPU가 더 효율적으로 일할 수 있습니다. `Renderer.cpp`의 `RenderLevel` 함수 내에서 이러한 정렬 로직이 구현되어있습니다.

#### 2.9.2. Draw Call 및 Render State

- **Draw Call 최소화**: Frustum Culling과 Occlusion Culling을 통해 화면에 보이지 않는 객체에 대한 Draw Call 자체를 원천적으로 차단하는 것이 가장 중요한 최적화입니다.
- **Render State 캐싱**: GPU 상태(Rasterizer, Blend, Depth-Stencil 등)를 변경하는 것은 비용이 큰 작업입니다.
    - `Source/Render/Renderer/Public/Renderer.h`: `URenderer` 클래스 내의 `RasterCache` (`TMap<FRasterKey, ID3D11RasterizerState*, ...>`)가 좋은 예시입니다.
    - Fill Mode, Cull Mode 등의 설정이 동일한 `ID3D11RasterizerState`가 요청되면, 새로 생성하지 않고 `TMap`에 캐싱된 기존 객체를 재사용합니다. 이는 `GetRasterizerState` 함수를 통해 구현되며, 불필요한 D3D11 API 호출과 객체 생성을 줄여줍니다.

---

## 3. 주요 시나리오 분석: 50,000개 StaticMesh Picking

5만 개의 `StaticMeshComponent`가 배치된 씬에서 특정 위치를 클릭하여 객체를 선택(Picking)하는 상황을 가정합니다.

- **Naive approach (느린 방법)**:
    1. 5만 개의 모든 `StaticMeshComponent`를 순회합니다.
    2. 각각의 컴포넌트에 대해, 클릭 지점에서 발사된 레이(Ray)와 객체의 모든 삼각형 간의 교차 검사를 수행합니다.
    3. 이는 `50,000 * (메쉬 당 삼각형 수)` 만큼의 엄청난 연산을 필요로 하며, 실시간으로 처리하는 것은 불가능에 가깝습니다.

- **GTL Engine의 최적화된 접근 방식 (계층적 가속 구조 활용)**:
    1. **Scene-Level (TLAS)**: `ObjectPicker::PickPrimitive` 함수는 먼저 레이를 `FSceneBVH`에 전달합니다. BVH 트리를 따라 내려가면서 레이와 전혀 겹치지 않는 수많은 객체들(의 AABB)이 매우 빠르게 제외됩니다. 이 단계가 끝나면 충돌 후보 객체는 5만 개에서 수십 개 이하로 줄어듭니다.
    2. **Mesh-Level (BLAS)**: 추려진 소수의 후보 객체(`StaticMeshComponent`) 각각에 대해, 이제 레이를 해당 객체의 로컬 공간으로 변환하여 `FStaticMeshBVH`와 교차 검사를 수행합니다. 이 단계에서 메쉬를 구성하는 수천, 수만 개의 삼각형 중 레이와 충돌 가능성이 있는 극소수의 삼각형만이 후보로 남게 됩니다.
    3. **Triangle-Level**: 마지막으로, 최종 후보로 남은 몇 개의 삼각형에 대해서만 정밀한 레이-삼각형 교차 검사(`IsRayTriangleCollided`)를 수행하여 정확한 충돌 지점과 거리를 계산합니다.

이러한 **SceneBVH → StaticMeshBVH → Triangle** 순서의 계층적(Hierarchical) 테스트는 검사 대상을 기하급수적으로 줄여나가기 때문에, 5만 개라는 엄청난 수의 객체 속에서도 실시간으로 정확한 Picking이 가능하게 됩니다. `FScopeCycleCounter`를 `PickPrimitive` 함수에 적용하여 이 과정의 성능을 측정하면, 최적화의 효과를 명확히 수치로 확인할 수 있습니다.

---

## 4. 결론

GTL Engine은 Frustum/Occlusion Culling, 메시 LOD, 계층적 Scene Graph, SIMD를 통한 수학 연산 가속, 그리고 목적에 맞는 공간 분할 자료구조(Octree, BVH)의 전략적 사용 등 다양한 고급 최적화 기법을 적극적으로 활용하고 있습니다. 특히 Picking 시나리오에서 볼 수 있듯이, BVH를 계층적으로 구성하여 대규모 씬에서의 레이캐스팅 성능을 극대화하고자 했습니다. 또한 RAII를 활용한 안전한 성능 측정 유틸리티와 Render State 캐싱 등을 통해 코드의 안정성과 효율성을 모두 고려해 설계했습니다.
