
////////////////////////////////////////////////////////////////////////////////////////////
//						Custom Monster Actor - By Kostas Dokos							  //
////////////////////////////////////////////////////////////////////////////////////////////

#include "MonsterCombat.h"
#include "CustomMonsterActor.h"
#include "CombatMeters.h"
#include "CombatController.h"


ACustomMonsterActor::ACustomMonsterActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MonsterCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MonsterCam"));
	MonsterCameraRoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MonsterCamRoot"));

	MonsterCamera->AttachTo(MonsterCameraRoot);
	
}

void ACustomMonsterActor::BeginPlay()
{
	Super::BeginPlay();
	
	InitialCameraRot = MonsterCameraRoot->RelativeRotation;
	ACombatController* TheController = Cast<ACombatController>(GetWorld()->GetFirstPlayerController());
	TurningSpeed = TheController->TurningSpeed;
	AnimInstance = Cast<UMonsterAnimInstance>(GetMesh()->GetAnimInstance());
	InitPos = LeInitPosActor->GetActorLocation();

	TArray<UWidgetComponent*> WidgetComps;
	GetComponents<UWidgetComponent>(WidgetComps);
	MyHealthWidgetComponent = WidgetComps[0];
	MyManaWidgetComponent = WidgetComps[1];

	for (size_t i = 0; i < 6; i++)
	{
		if (MonsterType == TheController->MonsterStats[i].MonsterType)
		{
			MaxHealth = TheController->MonsterStats[i].MaxHP;
			MaxMana = TheController->MonsterStats[i].MaxMP;

			AI_Archetype = TheController->MonsterStats[i].AI_Archetype;
			

			Attack_Temp = 0;
			Defense_Temp = 0;
			MagicAttack_Temp = 0;
			MagicDefense_Temp = 0;

			CurrentHealth = MaxHealth;
			CurrentMana = MaxMana;

			Speed = TheController->MonsterStats[i].Speed;
			TheController->MyActionHUD->MonsterNames[MonsterID] = TheController->MonsterStats[i].MonsterNames[FMath::RandRange(0, TheController->MonsterStats[i].MonsterNames.Num() - 1)];
			MonsterName = TheController->MyActionHUD->MonsterNames[MonsterID];

			TheController->MyActionHUD->MonsterHPs_Current[MonsterID] = CurrentHealth;
			TheController->MyActionHUD->MonsterHPs_Max[MonsterID] = MaxHealth;
			TheController->MyActionHUD->MonsterMPs_Current[MonsterID] = CurrentMana;
			TheController->MyActionHUD->MonsterMPs_Max[MonsterID] = MaxMana;
			TheController->MyActionHUD->MonsterPortraits[MonsterID] = TheController->MonsterStats[i].MonsterPortrait;

			MonsterPortrait = TheController->MonsterStats[i].MonsterPortrait;

			Attack = TheController->MonsterStats[i].Attack;
			Defense = TheController->MonsterStats[i].Defence;
			MagicAttack = TheController->MonsterStats[i].SpecialAttack;
			MagicDefense = TheController->MonsterStats[i].SpecialDefence;

			HitChance = TheController->MonsterStats[i].HitChance;
			AttackCritChance = TheController->MonsterStats[i].CritChance;
			AttackCritMultiplier = TheController->MonsterStats[i].CritMultiplier;

			AttackBonus_Max = TheController->MonsterStats[i].DmgBonus_Max;
			AttackBonus_Min = TheController->MonsterStats[i].DmgBonus_Min;

			MonsterAffinityType = TheController->MonsterStats[i].MonsterAffinityType;

			Spells = TheController->MonsterStats[i].MonsterSpellbook.Abilities;

		}
	}
	
	AllDone = true;
}

void ACustomMonsterActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	MonsterCameraRoot->AddLocalRotation(FRotator(0, DeltaTime * TurningSpeed, 0));

	if (bIsDead)
	{
		StatusAbnormalitiesTextSweep = "Dead";
	}
	else if (ActiveAbnormalities.Num() == 0)
	{
		StatusAbnormalitiesTextSweep = "";
	}
	else if(ActiveAbnormalities.Num() == 1)
	{
		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EStatusAbnormality"), true);
		FString String = EnumPtr->GetEnumName(ActiveAbnormalities[0]);

		StatusAbnormalitiesTextSweep = String;
	}
	else
	{
		AbnormalityTimer += DeltaTime;
		if (AbnormalityTimer > 2)
		{
			AbnormalityTimer = 0;

			const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EStatusAbnormality"), true);
			FString String = EnumPtr->GetEnumName(ActiveAbnormalities[AbnormalityIndex]);

			if (AbnormalityIndex + 1 < ActiveAbnormalities.Num())
				AbnormalityIndex++;
			else
				AbnormalityIndex = 0;

			StatusAbnormalitiesTextSweep = String;
		}
	}

}

////////////////////////////////////////////////////////////////////////////////////////////
//								Monster Gameplay Related								  //
////////////////////////////////////////////////////////////////////////////////////////////

void ACustomMonsterActor::MonsterDeath()
{
	bIsDead = true;
	AnimInstance->bIsDead = bIsDead;

	ActiveAbnormalities.Empty();
	ActiveBuffs.Empty();

	ACombatController* TheController = Cast<ACombatController>(GetWorld()->GetFirstPlayerController());
	TheController->CheckForWinConditions();
}

void ACustomMonsterActor::MonsterRez()
{
	bIsDead = false;
	AnimInstance->bIsDead = bIsDead;
}

void ACustomMonsterActor::UpdateHealth(bool WillCrit, int32 Amount)
{
	if (Amount == 0)return;

	CurrentHealth += Amount;

	if(Amount<0)
		Cast<UCombatMeters>(MyHealthWidgetComponent->GetUserWidgetObject())->CreateNumberOverHead(WillCrit, FLinearColor(1, 0, 0, 1), FString::FromInt(Amount));
	else
		Cast<UCombatMeters>(MyHealthWidgetComponent->GetUserWidgetObject())->CreateNumberOverHead(WillCrit, FLinearColor(0, 1, 0, 1), FString::FromInt(Amount));
			
	if (CurrentHealth > MaxHealth)
	{
		CurrentHealth = MaxHealth;
	}
	else if (CurrentHealth <= 0)
	{
		CurrentHealth = 0;
		MonsterDeath();
	}
	
	ACombatController* TheController = Cast<ACombatController>(GetWorld()->GetFirstPlayerController());
	TheController->MyActionHUD->MonsterHPs_Current[MonsterID] = CurrentHealth;

}

void ACustomMonsterActor::UpdateMana(bool WillCrit, int32 Amount)
{
	if (Amount == 0)return;

	CurrentMana += Amount;

	if (Amount<0)
		Cast<UCombatMeters>(MyManaWidgetComponent->GetUserWidgetObject())->CreateNumberOverHead(WillCrit, FLinearColor(1, 0, 1, 1), "" + FString::FromInt(Amount));
	else
		Cast<UCombatMeters>(MyManaWidgetComponent->GetUserWidgetObject())->CreateNumberOverHead(WillCrit, FLinearColor(0, 1, 1, 1), "" + FString::FromInt(Amount));

	if (CurrentMana > MaxMana)
	{
		CurrentMana = MaxMana;
	}
	else if (CurrentMana <= 0)
	{
		CurrentMana = 0;
	}

	ACombatController* TheController = Cast<ACombatController>(GetWorld()->GetFirstPlayerController());
	TheController->MyActionHUD->MonsterMPs_Current[MonsterID] = CurrentMana;
}

