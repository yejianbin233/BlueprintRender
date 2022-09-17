// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RenderActor.generated.h"

UCLASS()
class BLUEPRINTRENDER_API ARenderActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Component", DisplayName="静态网格体组件")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Component", DisplayName="程序化网格体组件")
	class UProceduralMeshComponent* ProceduralMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Render Target", DisplayName="深度渲染目标")
	UTextureRenderTarget2D* RT_Depth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Texture", DisplayName="被绘制的纹理")
	UTexture* Texture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Camera", DisplayName="视野")
	float FOV=90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Camera", DisplayName="摄像机变换")
	FTransform CameraTransform;
	
public:	
	// Sets default values for this actor's properties
	ARenderActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

	UFUNCTION(BlueprintCallable, Category="Transform", DisplayName="将世界坐标转换成屏幕坐标")
	FVector2D WorldToScreen(FVector InWorldLocation, FVector2D InScreenSize, float InFOV);
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Draw", DisplayName="使用三角面数据绘制纹理到渲染目标")
	void TriangleDrawTextureToRenderTarget();
};
