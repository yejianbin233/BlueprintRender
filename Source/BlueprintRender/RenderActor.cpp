// Fill out your copyright notice in the Description page of Project Settings.


#include "RenderActor.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ARenderActor::ARenderActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));

	SetRootComponent(ProceduralMeshComponent);
	
	StaticMeshComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ARenderActor::BeginPlay()
{
	Super::BeginPlay();
}

FVector2D ARenderActor::WorldToScreen(FVector InWorldLocation, FVector2D InScreenSize, float InFOV)
{
	
	// 屏幕尺寸比
	float ScreenXYRatio = InScreenSize.Y / InScreenSize.X;

	// 深度
	float Depth = InWorldLocation.X;

	// UV
	FVector2D UV_XY(InWorldLocation.Y, InWorldLocation.Z);

	// 公式
	float Value = (ScreenXYRatio * UKismetMathLibrary::DegTan(InFOV / 2)) * Depth;

	FVector2D UV = UV_XY / Value;

	// 调整 UV 方向，原 Y 朝上，现乘以负数，将其朝下
	UV = UV * FVector2D(0.5, -0.5);

	// 将 UV 的原点偏移到屏幕中心
	FVector2D UVOffsetToCenter = UV + FVector2D(0.5, 0.5);

	return  UVOffsetToCenter;
}

// Called every frame
void ARenderActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARenderActor::TriangleDrawTextureToRenderTarget()
{
	// 清除无效的渲染目标数据
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), RT_Depth, FLinearColor(0,0,0,1));

	// 网格体的变换
	// note：需要在编辑器中将静态网格体偏移指定深度(X轴向前偏移 200)，否则不能正确显示渲染图像
	FTransform StaticMeshComponentRelativeTransform = StaticMeshComponent->GetRelativeTransform();

	// 将静态网格体的数据复制到程序化网格体组件中，然后才可以从程序化网格体得到网格体的数据(三角面，UV，切线等)
	UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(StaticMeshComponent, 0, ProceduralMeshComponent, false);

	// ??? 是否是两个方式都可以得到网格体数据，不需要将静态网格体转换成程序化网格体?
	// 顶点
	TArray<FVector> Vertices;
	// 三角面(保存相互独立的三角面点的索引)
	TArray<int32> Triangles;
	// 法线
	TArray<FVector> Normals;
	// UV
	TArray<FVector2D> UVs;
	// 切线
	TArray<FProcMeshTangent> Tangents;
	UKismetProceduralMeshLibrary::GetSectionFromProceduralMesh(ProceduralMeshComponent,
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		Tangents);
	// UKismetProceduralMeshLibrary::GetSectionFromStaticMesh()

	// 三角面数量(网格体一共有多少个三角面，遍历每个三角面进行绘制)
	// int32 TriangleNum = Triangles.Num() / 3;

	// 开始绘制渲染目标
	
	UCanvas* Canvas = NewObject<UCanvas>();
	FVector2D Size;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), RT_Depth, Canvas, Size, Context);

	// 遍历三角面(每三个一组)
	for(int32 I = 0; I < Triangles.Num(); I += 3)
	{
		// 越界判断
		if (Triangles.Num() <= I)
		{
			break;
		}
		
		const int32 TriangleA = Triangles[I];
		const int32 TriangleB = Triangles[I+1];
		const int32 TriangleC = Triangles[I+2];


		FVector VerticeA = Vertices[TriangleA];
		FVector VerticeB = Vertices[TriangleB];
		FVector VerticeC = Vertices[TriangleC];

		// 将网格体顶点的相对坐标转换为世界坐标
		FVector WorldLocationA = UKismetMathLibrary::TransformLocation(StaticMeshComponentRelativeTransform, VerticeA);
		FVector WorldLocationB = UKismetMathLibrary::TransformLocation(StaticMeshComponentRelativeTransform, VerticeB);
		FVector WorldLocationC = UKismetMathLibrary::TransformLocation(StaticMeshComponentRelativeTransform, VerticeC);

		// 将网格体顶点的世界坐标转换成摄像机的相对坐标
		FVector CameraRelativeLocationA =  UKismetMathLibrary::InverseTransformLocation(CameraTransform, WorldLocationA);
		FVector CameraRelativeLocationB =  UKismetMathLibrary::InverseTransformLocation(CameraTransform, WorldLocationB);
		FVector CameraRelativeLocationC =  UKismetMathLibrary::InverseTransformLocation(CameraTransform, WorldLocationC);

		FVector2D ScreenLocationA = Size * WorldToScreen(CameraRelativeLocationA, Size, FOV);
		FVector2D ScreenLocationB = Size * WorldToScreen(CameraRelativeLocationB, Size, FOV);
		FVector2D ScreenLocationC = Size * WorldToScreen(CameraRelativeLocationC, Size, FOV);
		
		TArray<FCanvasUVTri> NewTriangles;

		FCanvasUVTri UVTri;
		// 三角面的点的位置
		UVTri.V0_Pos = ScreenLocationA;
		UVTri.V1_Pos = ScreenLocationB;
		UVTri.V2_Pos = ScreenLocationC;

		// 临时的 UV(错误 - TODO)
		UVTri.V0_UV = UVs[TriangleA];
		UVTri.V1_UV = UVs[TriangleB];
		UVTri.V2_UV = UVs[TriangleC];

		// 颜色的颜色(默认则不会显示任何颜色)
		UVTri.V0_Color = FLinearColor(1,1,1,1);
		UVTri.V1_Color = FLinearColor(1,1,1,1);
		UVTri.V2_Color = FLinearColor(1,1,1,1);
		
		NewTriangles.Add(UVTri);
		
		if (Canvas && Texture)
		{
			// 使用渲染目标的画布画布绘制三角面
			Canvas->K2_DrawTriangle(Texture, NewTriangles);
		}
	}
	// 结束绘制渲染目标
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
}

