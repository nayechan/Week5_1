#pragma once
#include "Object.h"
#include "ObjectPtr.h"

class ULevel;
class AActor;
class FWorldContext;

/**
 * @brief World Type Enumeration
 */
enum class EWorldType : uint8
{
    Editor,
    EditorPreview, 
    PIE,
    Game,
};

/**
 * @brief World Context Structure
 * 각 월드의 컨텍스트 정보를 담는 구조체
 */
struct FWorldContext
{
    TObjectPtr<class UWorld> World;
    EWorldType WorldType = EWorldType::Editor;

    FWorldContext() = default;
    FWorldContext(UWorld* InWorld, EWorldType InWorldType) 
        : World(InWorld), WorldType(InWorldType) {}

    UWorld* GetWorld() const { return World.Get(); }
};

/**
 * @brief UWorld Class
 * 게임 월드를 관리하는 최상위 클래스
 */
UCLASS()
class UWorld : public UObject
{
    GENERATED_BODY()
    DECLARE_CLASS(UWorld, UObject)

public:
    UWorld();
    UWorld(const FName& InName);
    ~UWorld() override;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    /**
     * @brief 월드의 모든 액터를 업데이트
     * @param DeltaTime 이전 프레임으로부터의 시간
     */
    virtual void Tick(float DeltaTime);

    /**
     * @brief PIE용 월드 복제
     * @param EditorWorld 복제할 에디터 월드
     * @return 복제된 PIE 월드
     */
    static UWorld* DuplicateWorldForPIE(UWorld* EditorWorld);

    /**
     * @brief PIE 플레이를 위한 액터 초기화
     */
    void InitializeActorsForPlay();

    /**
     * @brief 월드 정리 및 해제
     */
    void CleanupWorld();

    /**
     * @brief PIE 월드인지 확인
     */
    bool IsPIEWorld() const { return WorldType == EWorldType::PIE; }

    /**
     * @brief 에디터 월드인지 확인
     */
    bool IsEditorWorld() const { return WorldType == EWorldType::Editor; }

    // Getters & Setters
    ULevel* GetLevel() const { return Level.Get(); }
    void SetLevel(ULevel* InLevel) { Level = InLevel; }
    
    EWorldType GetWorldType() const { return WorldType; }
    void SetWorldType(EWorldType InWorldType) { WorldType = InWorldType; }

private:
    // GTL에서는 Sub Level은 고려하지 않습니다.
    TObjectPtr<ULevel> Level = nullptr;
    EWorldType WorldType = EWorldType::Editor;
};